#include <vector>
#include <algorithm>
#include <string>
#include <limits>
#include <iostream>
#include <cstdio>

using std::vector;
using std::cin;
using std::pair;
using std::numeric_limits;
using std::make_pair;
using std::string;

enum { ALPHABET_SIZE = 26 };

const int infinum = numeric_limits<int>::max();

typedef unsigned char Symbol;

string sample;

/* Gets the letter by index*/
Symbol GetSymbol(int index) {
	return (index < 0) ? (-index - 1) : sample[index];
}


struct Link {
	int start;
	int end;
	int to;
	Link() {
		to = -1; /*invalid link by default*/
	}
	Link(int _start, int _end, int _to) {
		start = _start;
		end = _end;
		to = _to;
	}
	Symbol GetFirstSymbol() const {
		return GetSymbol(start);
	}
};

struct Vertex {
	vector<Link> links; /*state links*/
	int suffix; /*suffix link*/
	Vertex() {
		suffix = -1;
	}

	Link *FindLink(Symbol symbol) {
		for (size_t i = 0; i < links.size(); i++) {
			if (links[i].GetFirstSymbol() == symbol)
				return &links[i];
		}
		return NULL;
	}
};

vector<Vertex> tree; /*suffix tree*/
int root;
int dummy; /*parent of root*/

int CreateNewVertex() {
	int number_of_vertices = tree.size();
	tree.push_back(Vertex());
	return number_of_vertices;
}

/* to, from - two vertices
[start, end) - the word on the edge */
void CreateSuffixLink(int from, int start, int end, int to) {
	Link *link = tree[from].FindLink(GetSymbol(start));
	if (link != NULL)
		*link = Link(start, end, to);
	else
		tree[from].links.push_back(Link(start, end, to));
}

/*Function goes along the suffix link*/
int &GetSuffix(int vertex) {
	return tree[vertex].suffix;
}

void InitializeTree() {
	tree.clear();
	dummy = CreateNewVertex();
	root = CreateNewVertex();
	GetSuffix(root) = dummy;
	for(int i = 0; i < ALPHABET_SIZE; ++i) {
		CreateSuffixLink(dummy, -i - 1, -i, root);
	}
}

/* Canonizes the reference pair (vertex, (start, end)) of a state*/
pair<int, int> Canonize(int vertex, int start, int end) {
	if(end <= start) {
		return make_pair(vertex, start);
	} else {
		const Link *current = tree[vertex].FindLink(GetSymbol(start));
		while(end - start >= current->end - current->start) {
			start += current->end - current->start;
			vertex = current->to;
			if(end > start)
				current = tree[vertex].FindLink(GetSymbol(start));
		}
		return make_pair(vertex, start);
	}
}

/* Checks if there is a t-transition from the (probably implicit)
state (vertex, (start, end))*/
pair<bool, int> Split(int vertex, int start, int end, Symbol c) {
	if(end <= start) {
		return make_pair(tree[vertex].FindLink(c) != NULL, vertex);
	} else {
		Link current = *tree[vertex].FindLink(GetSymbol(start));
		if(c == GetSymbol(current.start + end - start))
			return make_pair(true, vertex);
		int middle = CreateNewVertex();
		CreateSuffixLink(vertex, current.start, current.start + end - start, middle);
		CreateSuffixLink(middle, current.start + end - start, current.end, current.to);
		return make_pair(false, middle);
	}
}

/*Creates new branches (vertex, (start, end)) - the active point*/
pair<int, int> Update(int vertex, int start, int end) {
	pair<bool, int> splitResult;
	splitResult = Split(vertex, start, end, GetSymbol(end));
	int old_root = root;
	while(!splitResult.first) {
		CreateSuffixLink(splitResult.second, end, infinum, CreateNewVertex());
		if(old_root != root)
			GetSuffix(old_root) = splitResult.second;
		old_root = splitResult.second;
		pair<int, int> newPoint = Canonize(GetSuffix(vertex), start, end);
		vertex = newPoint.first;
		start = newPoint.second;
		splitResult = Split(vertex, start, end, GetSymbol(end));
	}
	if(old_root != root)
		GetSuffix(old_root) = splitResult.second;
	return make_pair(vertex, start);
}

/* Builds the whole suffix tree for the string sample*/
void Ukkonen() {
	InitializeTree();
	pair<int, int> activePoint = make_pair(root, 0);
	for(size_t i = 0; i < sample.length(); ++i) {
		activePoint = Update(activePoint.first, activePoint.second, i);
		activePoint = Canonize(activePoint.first, activePoint.second, i + 1);
	}
}

int ComputeDistinctSubstrings() {
	for (size_t i = 0; i < sample.size(); ++i)
		sample[i] -= 'a';
	Ukkonen();
	int sum_of_edge_lengths = 0;
	for (size_t vertex_id = 0; vertex_id < tree.size(); ++vertex_id) {
		if (vertex_id == dummy)
			continue;
		
		const Vertex &vertex = tree[vertex_id];
		for (size_t i = 0; i < vertex.links.size(); i++) {
			const Link &CreateSuffixLink = vertex.links[i];
			if (CreateSuffixLink.to == -1) /*denotes a non-existing link*/
				continue;
			if (CreateSuffixLink.end == infinum)
				sum_of_edge_lengths += sample.size() - CreateSuffixLink.start;
			else
				sum_of_edge_lengths += CreateSuffixLink.end - CreateSuffixLink.start;
		}
	}
	return sum_of_edge_lengths;
}

int main() {
	cin >> sample;
	printf("%d\n", ComputeDistinctSubstrings());
	return 0;
}

// vim: noet ts=8 sts=8 sw=8
