#include <Node.h>

Node::Node(Node* parent, const char* name) : parent(parent) {
	this->name = String(name);
	this->nodes = new KeyValueMap<Node>();
	this->methods = new KeyValueMap<Method>();

	Method* state = new Method(std::bind(&Node::state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	this->methods->set("state", state);

	Method* save = new Method(std::bind(&Node::save, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	this->methods->set("save", save);
}

Node::~Node() {
	delete this->nodes;
}


