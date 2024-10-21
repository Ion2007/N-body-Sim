#pragma once
#include "particle.h"
#include <vector>
#include <set>
struct Node
{
	Particle particle;
	Vector3 center = Vector3(0, 0, 0);
	Vector3 centerMass = Vector3(0, 0, 0);
	int depth;
	double mass;

	std::vector<Node*> children;
	Node() {

	}
	Node(Vector3& center, Particle& particle)
		: center(center),particle(particle) {
		children = std::vector<Node*>(8, nullptr);

	}
	~Node() {
		// Recursively delete all child nodes to free memory
		for (Node* child : children) {
			delete child;
		}

	}


};
struct Tree
{
	double boundry;
	const int MAX_DEPTH = 20;
	double thresh;
	double gravityConstant;
	std::vector<Particle>& particleList;
	Node* root;
	Tree(std::vector<Particle>& particleList, double boundry, double thresh, double gravityConstant);
	void add(Node*& curr, Particle& particle, Vector3 center, int depth);
	void makeTree();
	void particleGravity(Particle& p1, Particle& p2);
	void calculateGravity(Node*& curr, Particle& p);
	void nodeGravity(Particle& p1, Node& n1);
	void recursionHelp(Node& curr, double x, double y, double z);
	void gravity();
	~Tree() {
		delete root;  // This will trigger the Node destructor, which will recursively delete child nodes

	}

};

