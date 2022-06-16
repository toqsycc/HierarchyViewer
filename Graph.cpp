#include "Graph.hpp"

UniversalType::UniversalType() :
	num(0),
	depth(0),
	isOpen(false),
	type((bool)UniversalObjects::folder),
	hasParent(false),
	depthCorrupted(false),
	isHighlighted(false)
{

}

UniversalType::~UniversalType()
{
	memset(this, NULL, sizeof(*this));
}