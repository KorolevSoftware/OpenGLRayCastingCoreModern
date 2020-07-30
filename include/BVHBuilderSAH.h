#pragma once
#include <vector>
#include <glm.hpp>
#include <list>
#include <array>
#include "TextureGL.h"
struct AABBTmp
{
	AABBTmp();
	AABBTmp(glm::vec3 min, glm::vec3 max, glm::vec3 center, int triangleIndex);
	float getSAH();
	void surrounding(AABBTmp const& aabb);

	glm::vec3 min;
	glm::vec3 max;
	glm::vec3 center;
	int triangleIndex;
};

struct BVHNode {
	AABBTmp aabb;
	virtual bool IsLeaf() = 0; // pure virtual
};

struct BVHInner : BVHNode {
	BVHNode* left;
	BVHNode* right;
	virtual bool IsLeaf();
};

struct BVHLeaf : BVHNode {
	std::list<int> trianglesIndexs;
	virtual bool IsLeaf();
};


struct TriangleTmp
{
	TriangleTmp();
	TriangleTmp(glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 vertex3, int index);
	AABBTmp genAABB();
private:
	int index;
	glm::vec3 vertex1;
	glm::vec3 vertex2;
	glm::vec3 vertex3;
};
struct CacheFriendlyBVHNode {
	// parameters for leafnodes and innernodes occupy same space (union) to save memory
	// top bit discriminates between leafnode and innernode
	// no pointers, but indices (int): faster
	int isLeaf;
	union {
		// inner node - stores indexes to array of CacheFriendlyBVHNode
		struct {
			unsigned _idxLeft;
			unsigned _idxRight;
		} inner;
		// leaf node: stores triangle count and starting index in triangle list
		struct {
			unsigned _count; // Top-most bit set, leafnode if set, innernode otherwise
			unsigned _startIndexInTriIndexList;
		} leaf;
	} u;
	// bounding box
	glm::vec3 min;
	glm::vec3 max;
};

class BVHBuilderSAH
{
public:
	BVHBuilderSAH();
	void build(std::vector<float> const& vertexRaw);
	BVHNode* recurseBuild(std::vector<AABBTmp> const& work, int depth);
	TextureGL indexToTexture();
	TextureGL bvhToTexture();


private:
	unsigned CountTriangles(BVHNode* root);
	void CountDepth(BVHNode* root, int depth, int& maxDepth);
	int CountBoxes(BVHNode* root);
	void CreateCFBVH();
	void PopulateCacheFriendlyBVH(BVHNode* root, unsigned& idxBoxes, unsigned& idxTriList);
	
	std::vector<TriangleTmp> vecTriangle;
	BVHNode* root;
	unsigned g_pCFBVH_No;
	unsigned g_triIndexListNo;
	int* g_triIndexList;
	CacheFriendlyBVHNode* g_pCFBVH;
};

