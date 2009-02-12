#include <iostream>
#include <fstream>
#include <acotreewidth/decomp.h>

EliminationGraph::EliminationGraph(const Graph &graph) {
  size__ = graph.number_of_vertices();
  a__ = new int*[size__];
  e__ = new int*[size__];
  t__ = new bool*[size__];
  eliminated__ = new bool[size__];
  memset(eliminated__,false,sizeof(bool)*size__);
  nr_eliminations__ = 0;

  for(unsigned int k=0;k<size__;k++) {
    a__[k] = new int[size__];
    e__[k] = new int[size__+1];
    t__[k] = new bool[size__];
    memset(t__[k],false,sizeof(bool)*size__);
  }


  for(unsigned int i=0;i<size__;i++) {
    std::vector<unsigned int> neighbours = graph.get_neighbours(i);
    for(unsigned int j=0;j<neighbours.size();j++) {
      a__[i][j] = neighbours[j];
      t__[i][neighbours[j]] = true;
      t__[neighbours[j]][i] = true;
    }
    e__[i][0] = neighbours.size();
  }
}

EliminationGraph::~EliminationGraph() {
  for(unsigned int i=0;i<size__;i++) {
    delete[] a__[i];
    delete[] e__[i];
    delete[] t__[i];
  }
  delete[] a__;
  delete[] e__;
  delete[] t__;
  delete[] eliminated__;
}

void EliminationGraph::eliminate(unsigned int vertex) {
  for(unsigned int k=0;k<size__;k++) {
    e__[k][nr_eliminations__+1] = e__[k][nr_eliminations__];
  }

  for(int i=0;i<e__[vertex][nr_eliminations__];i++) {
    for(int j=i+1;j<e__[vertex][nr_eliminations__];j++) {
      int n1 = a__[vertex][i];
      int n2 = a__[vertex][j];
      if(!eliminated__[n1] && !eliminated__[n2] && !t__[n1][n2]) {
        t__[n1][n2] = true;
        t__[n2][n1] = true;
        a__[n1][e__[n1][nr_eliminations__+1]] = n2;
        a__[n2][e__[n2][nr_eliminations__+1]] = n1;
        e__[n1][nr_eliminations__+1]++;
        e__[n2][nr_eliminations__+1]++;
      }
    }
    t__[vertex][i] = false;
    t__[i][vertex] = false;
  }

  nr_eliminations__++;
  eliminated__[vertex] = true;
}

unsigned int EliminationGraph::get_degree(unsigned int vertex) const {
  unsigned int degree = 0;
  for(int i=0;i<e__[vertex][nr_eliminations__];i++) {
    if(!eliminated__[a__[vertex][i]]) {
      degree++;
    }
  }
  return degree;
}

unsigned int EliminationGraph::min_fill(unsigned int vertex) const {
  unsigned int min_fill = 0;
  for(int i=0;i<(e__[vertex][nr_eliminations__]-1);i++) {
    for(int j=1;j<e__[vertex][nr_eliminations__];j++) {
      int n1 = a__[vertex][i];
      int n2 = a__[vertex][j];
      if(!eliminated__[n1] && !eliminated__[n2] && !t__[n1][n2]) {
        min_fill++;
      }
    }
  }
  return min_fill;
}

std::vector<unsigned int> EliminationGraph::get_neighbours(unsigned int vertex) const {
  std::vector<unsigned int> neighbours;
  for(int i=0;i<e__[vertex][nr_eliminations__];i++) {
    if(!eliminated__[a__[vertex][i]]) {
      neighbours.push_back(a__[vertex][i]);
    }
  }
  return neighbours;
}

unsigned int EliminationGraph::number_of_vertices() {
  return size__ - nr_eliminations__;
}

void EliminationGraph::rollback() {
  nr_eliminations__ = 0;
  memset(eliminated__,false,sizeof(bool)*size__);
  for(unsigned int k=0;k<size__;k++) {
    memset(t__[k],false,sizeof(bool)*size__);
  }

  for(unsigned int i=0;i<size__;i++) {
    for(unsigned int j=0;j<e__[i][nr_eliminations__];j++) {
      t__[i][a__[i][j]] = true;
      t__[a__[i][j]][i] = true;
    }
  }
}

DecompLocalSearch::DecompLocalSearch(std::vector<unsigned int> initial_solution, EvaluationFunction &eval_func, Neighbourhood &neighbourhood) : LocalSearch(initial_solution, eval_func, neighbourhood) {
}

void DecompLocalSearch::search_neighbourhood() {
  const std::vector<unsigned int> &solution = neighbourhood_->next_neighbour_solution();
  double quality = eval_func_->eval_solution(solution);
  if(quality > best_so_far_quality_) {
    best_so_far_solution_ = solution;
    best_so_far_quality_ = quality;
  }
  neighbourhood_->set_solution(best_so_far_solution_);
}

double Heuristic::min_degree(const EliminationGraph &graph, unsigned int vertex) {
  return 1.0 / (graph.get_degree(vertex) + 1);
}

double Heuristic::min_fill(const EliminationGraph &graph, unsigned int vertex) {
  return 1.0 / (graph.min_fill(vertex) + 1);
}

FileNotFoundException::FileNotFoundException(const char *filepath) {
  filepath_ = filepath;
}

const char *FileNotFoundException::what() const throw() {
  return filepath_;
}

HyperGraph &Parser::parse_hypertreelib(const char *filepath) throw(FileNotFoundException) {
  HyperGraph *graph = new HyperGraph();
  std::ifstream file(filepath);
  char buf[1024*8];

  if(!file) {
    throw FileNotFoundException(filepath);
  }

  std::map<std::string, unsigned int> vertex_ids;
  int edge_nr = 0;
  int vertex_nr = 0;
  while(file.good()) {
    std::string line;
    std::string atom;
    std::size_t found;
    file.getline(buf, 1024*8);
    line = std::string(buf);
    //strip whitespaces
    found = line.find(" ");
    while(found != std::string::npos) {
      line = line.replace(int(found), 1, "");
      found = line.find(" ");
    }

    if (line.length() > 0 && line[0] != '%' && line[0] != '\r' && line[0] != '<') {
      //strip ')' character
      found = line.find(")");
      line.replace(int(found), 1, "");
      //strip '.' character
      found = line.find(".");
      if(found != std::string::npos) {
        line.replace(int(found), 1, "");
      }

      found = line.find("(");
      atom = line.substr(0, int(found)); 
      line.replace(0, int(found)+1, "");
      found = line.find(",");
      std::vector<unsigned int> vars;
      while(found != std::string::npos) {
        std::string var = line.substr(0, int(found));
        if(vertex_ids.find(var) == vertex_ids.end()) {
          vars.push_back(vertex_nr);
          graph->set_vertex_label(vertex_nr, var);
          vertex_ids[var] = vertex_nr;
          vertex_nr++;
        } else {
          vars.push_back(vertex_ids[var]);
        }
        line.replace(0, int(found) + 1, "");
        found = line.find(",");
      }
      graph->add_hyperedge(edge_nr, vars);
      graph->set_edge_label(edge_nr, atom);
      edge_nr++;
    }
  }

  file.close();
  return *graph;
}