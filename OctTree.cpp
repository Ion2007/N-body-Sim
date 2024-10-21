#include "particle.h"
#include <vector>
#include <set>
#include <iostream>

struct Node {
    Particle particle;
    Vector3 center = Vector3(0, 0, 0);
    Vector3 centerMass = Vector3(0, 0, 0);
    int depth;
    double mass = 0.0;

    std::vector<Node*> children;

    Node() {}

    Node(Vector3& center, Particle& particle)
        : center(center), particle(particle), mass(particle.mass), centerMass(particle.pos) {
        children = std::vector<Node*>(8, nullptr);
    }

    ~Node() {
        #pragma omp parallel for
        for (Node* child : children) {
            delete child;
        }
    }
};

struct Tree {
    double boundry;
    const int MAX_DEPTH = 7;
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

Tree::Tree(std::vector<Particle>& particleList, double boundry, double thresh, double gravityConstant)
    : particleList(particleList), boundry(boundry), thresh(thresh), gravityConstant(gravityConstant) {}

void Tree::particleGravity(Particle& p1, Particle& p2) {
    if (&p1 == &p2) return;
    Vector3 displacement = p1.pos.difference(p2.pos);
    Vector3 unit = displacement.unit();
    double r = displacement.magnitude();

    if (r < .001 || r > -.001) {
        r += .01;
    }

    double forceMagnitude = (gravityConstant * p1.mass * p2.mass) / (r * r);
    double fx = forceMagnitude * unit.x;
    double fy = forceMagnitude * unit.y;
    double fz = forceMagnitude * unit.z;

    p1.velocity.x += fx / p1.mass;
    p1.velocity.y += fy / p1.mass;
    p1.velocity.z += fz / p1.mass;

    p2.velocity.x -= fx / p2.mass;
    p2.velocity.y -= fy / p2.mass;
    p2.velocity.z -= fz / p2.mass;
}

void Tree::nodeGravity(Particle& p1, Node& n1) {
    Vector3 displacement = p1.pos.difference(n1.centerMass);
    Vector3 unit = displacement.unit();
    double r = displacement.magnitude();

    if (r < .001 || r > -.001) {
        r += .01;
    }

    double forceMagnitude = (gravityConstant * p1.mass * n1.mass) / (r * r);
    double fx = forceMagnitude * unit.x;
    double fy = forceMagnitude * unit.y;
    double fz = forceMagnitude * unit.z;

    p1.velocity.x += fx / p1.mass;
    p1.velocity.y += fy / p1.mass;
    p1.velocity.z += fz / p1.mass;

    recursionHelp(n1, -fx, -fy, -fz);
}

void Tree::add(Node*& curr, Particle& particle, Vector3 center, int depth) {
    if (curr == nullptr) {
        curr = new Node(center, particle);
        curr->depth = depth + 1;
        return;
    }

    if (curr->particle.mass != 0 && curr->depth < MAX_DEPTH) {
        // If the current node has a particle and isn't subdivided, subdivide it
        Particle temp = curr->particle;
        curr->particle.mass = 0;  // Clear the particle to indicate it's no longer a leaf node

        // Update mass and center of mass
        curr->mass = temp.mass;
        curr->centerMass = temp.pos;

        add(curr, temp, center, depth);
    }

    // Update the mass and center of mass before adding the new particle
    double totalMass = curr->mass + particle.mass;
    curr->centerMass = (curr->centerMass * curr->mass + particle.pos * particle.mass) / totalMass;
    curr->mass = totalMass;

    // Determine the correct child node to place the new particle
    if (particle.pos.x == curr->center.x) particle.pos.x += .001;
    if (particle.pos.y == curr->center.y) particle.pos.y -= .001;
    if (particle.pos.z == curr->center.z) particle.pos.z -= .001;

    int index = 0;
    if (particle.pos.x > curr->center.x) index |= 1;
    if (particle.pos.y > curr->center.y) index |= 2;
    if (particle.pos.z > curr->center.z) index |= 4;

    Vector3 newCenter = curr->center;
    double offset = boundry / pow(2, depth);
    if (index & 1) newCenter.x += offset;
    else newCenter.x -= offset;
    if (index & 2) newCenter.y += offset;
    else newCenter.y -= offset;
    if (index & 4) newCenter.z += offset;
    else newCenter.z -= offset;

    // Recursively add the particle to the correct child node
    add(curr->children[index], particle, newCenter, curr->depth);
}

void Tree::makeTree() {
    root = new Node();
    root->center = Vector3(0, 0, 0); // Center of the boundary
    root->children = std::vector<Node*>(8, nullptr);
    root->depth = 1;
    root->particle.mass = 0;
    #pragma omp parallel for
    for (Particle& particle : particleList) {
        add(root, particle, root->center, 1);
    }
}

void Tree::recursionHelp(Node& curr, double x, double y, double z) {
    if (&curr == nullptr) {
        return;
    }
    bool isLeafNode = true;
    for (int i = 0; i < 8; i++) {
        if (curr.children[i] != nullptr) {
            isLeafNode = false;
            break;
        }
    }
    if (isLeafNode && curr.particle.mass != 0) {
        curr.particle.velocity.x += x / curr.particle.mass;
        curr.particle.velocity.y += y / curr.particle.mass;
        curr.particle.velocity.z += z / curr.particle.mass;
    }
    else {
        for (int i = 0; i < 8; i++) {
            if (curr.children[i] != nullptr) {
                recursionHelp(*curr.children[i], x, y, z);
            }
        }
    }
}

void Tree::calculateGravity(Node*& curr, Particle& p) {
    if (curr == nullptr) {
        return;  // Base case: if the node is null, do nothing
    }

    // Check if the current node is a leaf node (contains a particle and has no children)
    bool isLeafNode = true;
    for (int i = 0; i < 8; i++) {
        if (curr->children[i] != nullptr) {
            isLeafNode = false;
            break;
        }
    }

    if (isLeafNode && curr->particle.mass != 0) {
        // If this is a leaf node, apply particle gravity
        particleGravity(p, curr->particle);
    }
    else {
        // If this is not a leaf node, decide whether to treat the node as a single mass or recurse into children
        double distance = (curr->centerMass.difference(p.pos)).magnitude();
        double s = boundry / pow(2, curr->depth - 1);  // Size of the current node

        if (s / distance < thresh) {
            // If the node is sufficiently far away, treat it as a single mass
            nodeGravity(p, *curr);
        }
        else {
            // Otherwise, recurse into the children
            #pragma omp parallel for
            for (int j = 0; j < 8; j++) {
                if (curr->children[j] != nullptr) {
                    calculateGravity(curr->children[j], p);
                }
            }
        }
    }
}

void Tree::gravity() {
#pragma omp parallel for
    for (int i = 0; i < particleList.size(); i++) {
        calculateGravity(root, particleList[i]);
    }
}