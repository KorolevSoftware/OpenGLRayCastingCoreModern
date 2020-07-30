#include <functional>
#include <limits>
#include <iostream>
#include "BVHBuilderSAH.h"

using glm::vec3;

TriangleTmp::TriangleTmp() :TriangleTmp(vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 0, 0), -1) {}

TriangleTmp::TriangleTmp(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3, int index)
	:vertex1(vertex1), vertex2(vertex2), vertex3(vertex3), index(index) {}

AABBTmp::AABBTmp(glm::vec3 min, glm::vec3 max, glm::vec3 center, int triangleIndex)
	: min(min), max(max), center(center), triangleIndex(triangleIndex) {}

AABBTmp::AABBTmp() : AABBTmp(
	vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
	vec3(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()),
	vec3(0, 0, 0), -1) {}

BVHBuilderSAH::BVHBuilderSAH() { }

bool BVHInner::IsLeaf()
{
	return false;
}

bool BVHLeaf::IsLeaf()
{
	return true;
}

float AABBTmp::getSAH()
{
	vec3 length = max - min;// length bbox along axises
	return length.x * length.y + length.y * length.z + length.z * length.x;
}

void AABBTmp::surrounding(AABBTmp const& aabb)
{
	min = glm::min(min, aabb.min);
	max = glm::max(max, aabb.max);
}

AABBTmp TriangleTmp::genAABB()
{
	vec3 min = glm::min(glm::min(vertex1, vertex2), vertex3);
	vec3 max = glm::max(glm::max(vertex1, vertex2), vertex3);
	vec3 center = (min + max) * 0.5f;
	return AABBTmp(min, max, center, index);
}

void BVHBuilderSAH::build(std::vector<float> const& vertexRaw)
{
	int floatInTriangle = 9; // x,y,z x,y,z x,y,z = 9 float
	for (int index = 0; index < vertexRaw.size(); index += floatInTriangle)
	{
		vecTriangle.emplace_back(
			vec3(vertexRaw[index + 0], vertexRaw[index + 1], vertexRaw[index + 2]),
			vec3(vertexRaw[index + 3], vertexRaw[index + 4], vertexRaw[index + 5]),
			vec3(vertexRaw[index + 6], vertexRaw[index + 7], vertexRaw[index + 8]),
			index / floatInTriangle);
	}
	std::vector<AABBTmp> work;

	AABBTmp aabbNode = vecTriangle[0].genAABB();
	for(TriangleTmp& triangle: vecTriangle)
	{
		AABBTmp trianglAABB = triangle.genAABB();
		aabbNode.surrounding(trianglAABB);
		work.push_back(trianglAABB);
	}

	root = recurseBuild(work, 0); // builds BVH and returns root node
	root->aabb = aabbNode;

	CreateCFBVH();

}

BVHNode* BVHBuilderSAH::recurseBuild(std::vector<AABBTmp> const& work, int depth)
{
	// else, work size > 4, divide  node further into smaller nodes
	// start by finding the working list's bounding box (top and bottom)
	if (work.size() < 4)
	{
		BVHLeaf* leaf = new BVHLeaf;
		for (AABBTmp const& aabb : work)
			leaf->trianglesIndexs.push_back(aabb.triangleIndex);
		return leaf;
	}
	AABBTmp aabb = work[0];
	// loop over all bboxes in current working list, expanding/growing the working list bbox
	for (AABBTmp const& aabbTmp : work)
		aabb.surrounding(aabbTmp);

	// the current bbox has a cost of (number of triangles) * surfaceArea of C = N * SA
	float minCost = work.size() * aabb.getSAH();
	float bestSplit = FLT_MAX; // best split along axis, will indicate no split with better cost found (below)
	int bestAxis = -1;

	std::function<float(vec3 const& point)> getElement;

	using namespace std::placeholders;
	using vec3Getter = std::function<float(vec3 const& point)>;
    using std::bind;
	vec3Getter getterByAxis[3]{
		bind(&vec3::x, _1),
		bind(&vec3::y, _1),
		bind(&vec3::z, _1) };

	// Try all 3 axises X, Y, Z
	for (int axis = 0; axis < 3; axis++)
	{  // 0 = X, 1 = Y, 2 = Z axis
		// we will try dividing the triangles based on the current axis,
		// and we will try split values from "start" to "stop", one "step" at a time.
		float start, stop, step;
		vec3Getter getComponent = getterByAxis[axis];

		start = getComponent(aabb.min);
		stop = getComponent(aabb.max);

		// In that axis, do the bounding boxes in the work queue "span" across, (meaning distributed over a reasonable distance)?
		// Or are they all already "packed" on the axis? Meaning that they are too close to each other
		if (fabsf(stop - start) < 1e-4)
			// BBox side along this axis too short, we must move to a different axis!
			continue; // go to next axis

		// Binning: Try splitting at a uniform sampling (at equidistantly spaced planes) that gets smaller the deeper we go:
		// size of "sampling grid": 1024 (depth 0), 512 (depth 1), etc
		// each bin has size "step"
		step = (stop - start) / (1024.0f / (depth + 1.0f));


		// for each bin (equally spaced bins of size "step"):
		for (float testSplit = start + step; testSplit < stop - step; testSplit += step) {

			// Create left and right bounding box
			AABBTmp leftAABB;
			AABBTmp rightAABB;
			// The number of triangles in the left and right bboxes (needed to calculate SAH cost function)
			int countLeft = 0, countRight = 0;
			// For each test split (or bin), allocate triangles in remaining work list based on their bbox centers
			// this is a fast O(N) pass, no triangle sorting needed (yet)
			for (AABBTmp const& aabbTmp : work)
			{
				if (getComponent(aabbTmp.center) < testSplit) 
				{
					leftAABB.surrounding(aabbTmp);
					countLeft++;
				}
				else
				{
					rightAABB.surrounding(aabbTmp);
					countRight++;
				}
			}

			// Now use the Surface Area Heuristic to see if this split has a better "cost"

			// First, check for stupid partitionings, ie bins with 0 or 1 triangles make no sense
			if (countLeft <= 1 || countRight <= 1)
				continue;

			// calculate total cost by multiplying left and right bbox by number of triangles in each
			float totalCost = leftAABB.getSAH() * countLeft + rightAABB.getSAH() * countRight;

			// keep track of cheapest split found so far
			if (totalCost < minCost)
			{
				minCost = totalCost;
				bestSplit = testSplit;
				bestAxis = axis;
			}
		} // end of loop over all bins
	} // end of loop over all axises

	// at the end of this loop (which runs for every "bin" or "sample location"), 
	// we should have the best splitting plane, best splitting axis and bboxes with minimal traversal cost

	// If we found no split to improve the cost, create a BVH leaf
	if (bestAxis == -1)
	{
		std::cout << "bedAxis " << work.size() << std::endl;
		BVHLeaf* leaf = new BVHLeaf;
		for (AABBTmp const& aabbTmp : work)
			leaf->trianglesIndexs.push_back(aabbTmp.triangleIndex); // put triangles of working list in leaf's triangle list
		return leaf;
	}

	// Otherwise, create BVH inner node with L and R child nodes, split with the optimal value we found above
	AABBTmp leftAABB;
	std::vector<AABBTmp> left;

	AABBTmp rightAABB;
	std::vector<AABBTmp> right;

	// distribute the triangles in the left or right child nodes
	// for each triangle in the work set
	vec3Getter getComponent = getterByAxis[bestAxis];
	for (AABBTmp const& aabbTmp : work)
	{
		if (getComponent(aabbTmp.center) < bestSplit)
		{
			leftAABB.surrounding(aabbTmp);
			left.push_back(aabbTmp);
		}
		else
		{
			rightAABB.surrounding(aabbTmp);
			right.push_back(aabbTmp);
		}
	}

	// create inner node
	BVHInner* inner = new BVHInner;
	//inner->aabb = aabb;
	// recursively build the left child
	inner->left = recurseBuild(left,  depth + 1);
	inner->left->aabb = leftAABB;

	// recursively build the right child
	inner->right = recurseBuild(right, depth + 1);
	inner->right->aabb = rightAABB;
	return inner;
}

TextureGL BVHBuilderSAH::indexToTexture()
{
	return TextureGL(TextureGLType::TextureBufferX, g_triIndexListNo * sizeof(int), g_triIndexList, TextureGLDataFormat::INTEGER);
}

TextureGL BVHBuilderSAH::bvhToTexture()
{
	return TextureGL(TextureGLType::TextureBufferXYZ, g_pCFBVH_No * sizeof(CacheFriendlyBVHNode), g_pCFBVH, TextureGLDataFormat::FLOAT);
}

int BVHBuilderSAH::CountBoxes(BVHNode* root)
{
	if (!root->IsLeaf()) {
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		return 1 + CountBoxes(p->left) + CountBoxes(p->right);
	}
	else
		return 1;
}

// recursively count triangles
unsigned BVHBuilderSAH::CountTriangles(BVHNode* root)
{
	if (!root->IsLeaf()) {
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		return CountTriangles(p->left) + CountTriangles(p->right);
	}
	else {
		BVHLeaf* p = dynamic_cast<BVHLeaf*>(root);
		return (unsigned)p->trianglesIndexs.size();
	}
}

// recursively count depth
void BVHBuilderSAH::CountDepth(BVHNode* root, int depth, int& maxDepth)
{
	if (maxDepth < depth)
		maxDepth = depth;
	if (!root->IsLeaf()) {
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		CountDepth(p->left, depth + 1, maxDepth);
		CountDepth(p->right, depth + 1, maxDepth);
	}
}

void BVHBuilderSAH::CreateCFBVH()
{
	unsigned idxTriList = 0;
	unsigned idxBoxes = 0;

	g_triIndexListNo = CountTriangles(root);
	g_triIndexList = new int[g_triIndexListNo];

	g_pCFBVH_No = CountBoxes(root);
	g_pCFBVH = new CacheFriendlyBVHNode[g_pCFBVH_No]; // array

	PopulateCacheFriendlyBVH(root, idxBoxes, idxTriList);

	if ((idxBoxes != g_pCFBVH_No - 1) || (idxTriList != g_triIndexListNo)) {
		puts("Internal bug in CreateCFBVH, please report it..."); fflush(stdout);
		exit(1);
	}

	int maxDepth = 0;
	CountDepth(root, 0, maxDepth);
	if (maxDepth >= 32) {
		printf("Max depth of BVH was %d\n", maxDepth);
		puts("Recompile with BVH_STACK_SIZE set to more than that..."); fflush(stdout);
		exit(1);
	}
}

void BVHBuilderSAH::PopulateCacheFriendlyBVH(BVHNode* root, unsigned& idxBoxes, unsigned& idxTriList)
{
	unsigned currIdxBoxes = idxBoxes;
	g_pCFBVH[currIdxBoxes].min = root->aabb.min;
	g_pCFBVH[currIdxBoxes].max = root->aabb.max;

	//DEPTH FIRST APPROACH (left first until complete)
	if (!root->IsLeaf()) { // inner node
		BVHInner* p = dynamic_cast<BVHInner*>(root);
		// recursively populate left and right
		int idxLeft = ++idxBoxes;
		PopulateCacheFriendlyBVH(p->left, idxBoxes, idxTriList);
		int idxRight = ++idxBoxes;
		PopulateCacheFriendlyBVH( p->right, idxBoxes, idxTriList);
		g_pCFBVH[currIdxBoxes].isLeaf = 0;//
		g_pCFBVH[currIdxBoxes].u.inner._idxLeft = idxLeft;
		g_pCFBVH[currIdxBoxes].u.inner._idxRight = idxRight;
	}

	else 
	{ // leaf
		BVHLeaf* p = dynamic_cast<BVHLeaf*>(root);
		unsigned count = (unsigned)p->trianglesIndexs.size();
		g_pCFBVH[currIdxBoxes].isLeaf = 1;//
		g_pCFBVH[currIdxBoxes].u.leaf._count =  count;  // highest bit set indicates a leaf node (inner node if highest bit is 0)
		g_pCFBVH[currIdxBoxes].u.leaf._startIndexInTriIndexList = idxTriList;

		for (int triangleIndex : p->trianglesIndexs)
		{
			g_triIndexList[idxTriList++] = triangleIndex;
		}
	}
}
