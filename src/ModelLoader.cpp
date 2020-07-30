#include "ModelLoader.h"
#include <iostream>
#include <fstream>
#include <glm.hpp>
#include "Utils.h"

void ModelLoader::Obj(std::string const& filePath, std::vector<float>& rawVertex, std::vector<float>& rawNormal, std::vector<float>& rawUV)
{
    rawVertex.clear();
    rawNormal.clear();
    rawUV.clear();

    std::vector<float> tempVertex;
    std::vector<float> tempNormal;
    std::vector<float> tempuv;

    auto setData = [](int startIndex, int count, std::vector<float>& sourceBuffer, std::vector<float>& forBuffer)
    {
        int vertexIndex = startIndex - 1;
        float* pVertex = &sourceBuffer[vertexIndex * count];
        forBuffer.push_back(pVertex[0]);
        forBuffer.push_back(pVertex[1]);
        if (count == 3)
            forBuffer.push_back(pVertex[2]);
    };

    std::ifstream stream(Utils::resourceDir + filePath);
    if (!stream.is_open())
    {
        std::cerr << "error load file " + filePath << std::endl;
        return;
    }

    float x, y, z;
    int v1, t1, n1, v2, t2, n2, v3, t3, n3;
    std::string s;

    while (getline(stream, s))
    {
        if (sscanf(s.c_str(), "v %f %f %f", &x, &y, &z))
        {
            tempVertex.push_back(x);
            tempVertex.push_back(y);
            tempVertex.push_back(z);
        }

        if (sscanf(s.c_str(), "vt %f %f", &x, &y))
        {
            tempuv.push_back(x);
            tempuv.push_back(y);
        }

        if (sscanf(s.c_str(), "vn %f %f %f", &x, &y, &z))
        {
            tempNormal.push_back(x);
            tempNormal.push_back(y);
            tempNormal.push_back(z);
        }

        if (sscanf(s.c_str(), "f %i/%i/%i %i/%i/%i %i/%i/%i", &v1, &t1, &n1, &v2, &t2, &n2, &v3, &t3, &n3))
        {
            setData(v1, 3, tempVertex, rawVertex);
            setData(v2, 3, tempVertex, rawVertex);
            setData(v3, 3, tempVertex, rawVertex);

            setData(t1, 2, tempuv, rawUV);
            setData(t2, 2, tempuv, rawUV);
            setData(t3, 2, tempuv, rawUV);

            setData(n1, 3, tempNormal, rawNormal);
            setData(n2, 3, tempNormal, rawNormal);
            setData(n3, 3, tempNormal, rawNormal);
        }
    }
}

std::vector<float> ModelLoader::rayTracingTrianglePreComplite(std::vector<float>& rawVertex)
{
    std::vector<float> triangleIntersectData;
    using glm::vec3;
    for (unsigned index = 0; index < rawVertex.size(); index += 9) {
        //std::cout << index << std::endl;
        // Algorithm for triangle intersection is taken from Roman Kuchkuda's paper.
        // precompute edge vectors
        vec3 vertex1(rawVertex[index + 0], rawVertex[index + 1], rawVertex[index + 2]);
        vec3 vertex2(rawVertex[index + 3], rawVertex[index + 4], rawVertex[index + 5]);
        vec3 vertex3(rawVertex[index + 6], rawVertex[index + 7], rawVertex[index + 8]);

        vec3 vc1 = vertex2 - vertex1;
        vec3 vc2 = vertex3 - vertex2;
        vec3 vc3 = vertex1 - vertex3;

        // plane of triangle, cross product of edge vectors vc1 and vc2
        vec3 normal = glm::cross(vc1, vc2);

        // choose longest alternative normal for maximum precision
        vec3 alt1 = glm::cross(vc2, vc3);
        if (alt1.length() > normal.length())
            normal = alt1; // higher precision when triangle has sharp angles

        vec3 alt2 = glm::cross(vc3, vc1);
        if (alt2.length() > normal.length())
            normal = alt2;

        normal = glm::normalize(normal);
        vec3 e1 = glm::normalize(glm::cross(normal, vc1));
        vec3 e2 = glm::normalize(glm::cross(normal, vc2));
        vec3 e3 = glm::normalize(glm::cross(normal, vc3));
        // precompute dot product between normal and first triangle vertex

        triangleIntersectData.push_back(normal.x);
        triangleIntersectData.push_back(normal.y);
        triangleIntersectData.push_back(normal.z);
        triangleIntersectData.push_back(glm::dot(normal, vertex1));

        triangleIntersectData.push_back(e1.x);
        triangleIntersectData.push_back(e1.y);
        triangleIntersectData.push_back(e1.z);
        triangleIntersectData.push_back(glm::dot(e1, vertex1));

        triangleIntersectData.push_back(e2.x);
        triangleIntersectData.push_back(e2.y);
        triangleIntersectData.push_back(e2.z);
        triangleIntersectData.push_back(glm::dot(e2, vertex2));

        triangleIntersectData.push_back(e3.x);
        triangleIntersectData.push_back(e3.y);
        triangleIntersectData.push_back(e3.z);
        triangleIntersectData.push_back(glm::dot(e3, vertex3));
        //std::cout << triangleIntersectData.size() << std::endl;

        //// edge planes
        //triangle._e1 = cross(triangle._normal, vc1);
        //triangle._e1.normalize();
        //triangle._d1 = dot(triangle._e1, g_vertices[triangle._idx1]);
        //triangle._e2 = cross(triangle._normal, vc2);
        //triangle._e2.normalize();
        //triangle._d2 = dot(triangle._e2, g_vertices[triangle._idx2]);
        //triangle._e3 = cross(triangle._normal, vc3);
        //triangle._e3.normalize();
        //triangle._d3 = dot(triangle._e3, g_vertices[triangle._idx3]);
    }

    return triangleIntersectData;
}
