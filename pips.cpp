#include <cstdint>
#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <ostream>
#include <limits>

void die(const std::string& msg) {
  std::cerr << msg << std::endl;
  exit(-1);
}

template <typename T>
class Grid {
public:
  Grid(int num_rows, int num_cols, const T& default_value = T())
    : num_cols_(num_cols), cells_(num_rows * num_cols, default_value) {}

  void Set(int row, int col, const T& value) {
    cells_[row * num_cols_ + col] = value;
  }

  T Get(int row, int col) const {
    return cells_[row * num_cols_ + col];
  }

  bool Valid(int row, int col) const {
    return
      0 <= row && row < num_rows() &&
      0 <= col && col < num_cols();
  }

  int num_rows() const { return cells_.size() / num_cols_; }
  int num_cols() const { return num_cols_; }
  
private:
  std::vector<T> cells_;
  int num_cols_;
};

using CellId = uint8_t;
using DominoId = uint8_t;
using ConstraintId = uint8_t;
using Value = uint8_t;

struct Domino {
  DominoId id;
  Value lower_value;
  Value upper_value;
};

struct Position {
  CellId lower_cell;
  CellId upper_cell;
};

struct Move {
  Position position;
  DominoId domino;
  bool flipped;
};

struct Identical {};
struct Distinct {};
struct SumEqualTo {
  Value value;
};
struct SumLessThan {
  Value value;
};
struct SumGreaterThan {
  Value value;
};
using ConstraintType = std::variant<Identical, Distinct, SumEqualTo, SumLessThan, SumGreaterThan>;

struct Constraint {
  ConstraintId id;
  ConstraintType type;
  std::vector<CellId> cells;
};

void PrintCharGrid(const Grid<char>& print_grid) {
  for (int r = 0; r < print_grid.num_rows(); ++r) {
    std::string line(print_grid.num_cols(), ' ');
    for (int c = 0; c < print_grid.num_cols(); ++c) {
      line[c] = print_grid.Get(r, c);
    }
    std::cout << line << std::endl;
  }
}

void PrintCharGridsLeftToRight(const std::vector<Grid<char>>& print_grids,
			       int padding = 8) {
  if (print_grids.empty()) {
    return;
  }

  // Find the dimensions of the overall grid.

  const int num_rows = print_grids[0].num_rows();
  for (int i = 1; i < print_grids.size(); ++i) {
    if (print_grids[i].num_rows() != num_rows) {
      die("PrintCharGridsLeftToRight with mismatched rows");
    }
  }

  int num_cols = 0;
  for (const Grid<char>& print_grid : print_grids) {
    num_cols += print_grid.num_cols() + padding;
  }

  // Print each row.
  for (int r = 0; r < num_rows; ++r) {
    std::string line(num_cols, ' ');
    int offset = 0;
    for (const Grid<char>& print_grid : print_grids) {
      for (int c = 0; c < print_grid.num_cols(); ++c) {
	line[offset + c] = print_grid.Get(r, c);
      }
      offset += print_grid.num_cols() + padding;
    }
    std::cout << line << std::endl;
  }
}

struct Puzzle {
  Grid<std::optional<CellId>> grid;
  std::vector<Domino> dominos;
  std::vector<Constraint> constraints;

  void DebugPrint() {
    std::unordered_map<CellId, std::pair<int, int>> cell_to_rc;
    Grid<char> print_grid(grid.num_rows() * 4, grid.num_cols() * 5, ' ');
    for (int r = 0; r < grid.num_rows(); ++r) {
      for (int c = 0; c < grid.num_cols(); ++c) {
	const std::optional<CellId> cell = grid.Get(r, c);
	if (!cell.has_value()) {
	  continue;
	}
	cell_to_rc[*cell] = std::make_pair(r, c);
	print_grid.Set(r * 4,     c * 5,     '+');
	print_grid.Set(r * 4,     c * 5 + 1, '-');
	print_grid.Set(r * 4,     c * 5 + 2, '-');
	print_grid.Set(r * 4,     c * 5 + 3, '-');
	print_grid.Set(r * 4,     c * 5 + 4, '+');
	print_grid.Set(r * 4 + 1, c * 5,     '|');
	print_grid.Set(r * 4 + 1, c * 5 + 4, '|');
	print_grid.Set(r * 4 + 2, c * 5,     '|');
	print_grid.Set(r * 4 + 2, c * 5 + 4, '|');
	print_grid.Set(r * 4 + 3, c * 5,     '+');
	print_grid.Set(r * 4 + 3, c * 5 + 1, '-');
	print_grid.Set(r * 4 + 3, c * 5 + 2, '-');
	print_grid.Set(r * 4 + 3, c * 5 + 3, '-');
	print_grid.Set(r * 4 + 3, c * 5 + 4, '+');
	print_grid.Set(r * 4 + 1, c * 5 + 1, *cell / 100 + '0');
	print_grid.Set(r * 4 + 1, c * 5 + 2, *cell / 10 % 10 + '0');
	print_grid.Set(r * 4 + 1, c * 5 + 3, *cell % 10 + '0');
      }
    }

    // Print the grid with cell ids.
    PrintCharGrid(print_grid);
    std::cout << std::endl << std::endl;

    // Clear the cell ids.
    for (int r = 0; r < grid.num_rows(); ++r) {
      for (int c = 0; c < grid.num_cols(); ++c) {
	print_grid.Set(r * 4 + 1, c * 5 + 1, ' ');
	print_grid.Set(r * 4 + 1, c * 5 + 2, ' ');
	print_grid.Set(r * 4 + 1, c * 5 + 3, ' ');
      }
    }

    // Set up constraint labels.
    for (const Constraint& constraint : constraints) {
      for (const CellId& cell : constraint.cells) {
	const auto [r, c] = cell_to_rc[cell];
	print_grid.Set(r * 4 + 1, c * 5 + 1, constraint.id / 100 + '0');
	print_grid.Set(r * 4 + 1, c * 5 + 2, constraint.id / 10 % 10 + '0');
	print_grid.Set(r * 4 + 1, c * 5 + 3, constraint.id % 10 + '0');
	if (const Identical* equal = std::get_if<Identical>(&constraint.type)) {
	  print_grid.Set(r * 4 + 2, c * 5 + 2, '=');
	} else if (const Distinct* distinct = std::get_if<Distinct>(&constraint.type)) {
	  print_grid.Set(r * 4 + 2, c * 5 + 1, '!');
	  print_grid.Set(r * 4 + 2, c * 5 + 2, '=');
	} else if (const SumEqualTo* equal_to = std::get_if<SumEqualTo>(&constraint.type)) {
	  print_grid.Set(r * 4 + 2, c * 5 + 1, '=');
	  print_grid.Set(r * 4 + 2, c * 5 + 2, equal_to->value / 10 % 10 + '0');
	  print_grid.Set(r * 4 + 2, c * 5 + 3, equal_to->value % 10 + '0');
	} else if (const SumLessThan* less_than = std::get_if<SumLessThan>(&constraint.type)) {
	  print_grid.Set(r * 4 + 2, c * 5 + 1, '<');
	  print_grid.Set(r * 4 + 2, c * 5 + 2, less_than->value / 10 % 10 + '0');
	  print_grid.Set(r * 4 + 2, c * 5 + 3, less_than->value % 10 + '0');
	} else if (const SumGreaterThan* greater_than = std::get_if<SumGreaterThan>(&constraint.type)) {
	  print_grid.Set(r * 4 + 2, c * 5 + 1, '<');
	  print_grid.Set(r * 4 + 2, c * 5 + 2, greater_than->value / 10 % 10 + '0');
	  print_grid.Set(r * 4 + 2, c * 5 + 3, greater_than->value % 10 + '0');
	}
      }
    }

    // Print the grid with constraint labels.
    PrintCharGrid(print_grid);
    std::cout << std::endl << std::endl;

    for (const Domino& domino : dominos) {
      std::cout << static_cast<char>(domino.lower_value + '0')
		<< '|'
		<< static_cast<char>(domino.upper_value + '0')
		<< std::endl;
    }
  }
};

std::vector<std::string> ReadNonEmptyLines() {
  std::vector<std::string> lines;
  for (;;) {
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) {
      break;
    }
    lines.push_back(line);
  }
  return lines;
}

std::vector<std::string> Split(const std::string& str, const char delimiter) {
  std::vector<std::string> result;
  int current_start = 0;
  for (int i = 0; i < str.size() + 1; ++i) {
    if (i >= str.size() || str[i] == delimiter) {
      if (i > current_start) {
	result.push_back(str.substr(current_start, i - current_start));
      }
      current_start = i + 1;
    }
  }
  return result;
}

Value ParseValue(const std::string& str) {
  int x = 0;
  std::size_t chars_processed = 0;
  try {
    x = std::stoi(str, &chars_processed);
  } catch (...) {
    die("Couldn't convert to int: " + str);
  }

  if (chars_processed != str.size()) {
    die("Couldn't convert to int: " + str);
  }

  if (x < 0 || x > 255) {
    die("Value out of range: " + str);
  }

  return static_cast<Value>(x);
}

bool IsValue(const std::string& str) {
  int x = 0;
  std::size_t chars_processed = 0;
  try {
    x = std::stoi(str, &chars_processed);
  } catch (...) {
    return false;
  }
  return (chars_processed == str.size() &&
	  x >= 0 && x <= 255);
}

struct ConstraintMapping {
  std::vector<Constraint> constraints;
  std::unordered_map<char, ConstraintId> name_to_id;
};

ConstraintType ParseConstraintType(const std::string& type_string) {
  if (type_string == "=") {
    return Identical();
  }
  if (type_string == "!=") {
    return Distinct();
  }
  if (IsValue(type_string)) {
    return SumEqualTo{.value = ParseValue(type_string)};
  }
  if (type_string.size() > 1 &&
      type_string[0] == '<' &&
      IsValue(type_string.substr(1))) {
    return SumLessThan{.value = ParseValue(type_string.substr(1))};
  }
  if (type_string.size() > 1 &&
      type_string[0] == '>' &&
      IsValue(type_string.substr(1))) {
    return SumGreaterThan{.value = ParseValue(type_string.substr(1))};
  }
  die("Invalid constraint type: " + type_string);
}

ConstraintMapping ParseConstraints(const std::vector<std::string>& constraint_lines) {
  ConstraintMapping mapping;
  for (const std::string& line : constraint_lines) {
    const std::vector<std::string> parts = Split(line, ':');
    if (parts.size() != 2) {
      die("Invalid constraint: " + line);
    }
    if (parts[0].size() != 1) {
      die("Invalid constraint name: " + parts[0]);
    }
    const char name = parts[0][0];
    const ConstraintType constraint_type = ParseConstraintType(parts[1]);
    const ConstraintId id = mapping.constraints.size();
    mapping.constraints.push_back(Constraint{
	.id = id,
	.type = constraint_type,
      });
    mapping.name_to_id[name] = id;
  }
  return mapping;
}

std::vector<Domino> ParseDominos(const std::vector<std::string>& domino_lines) {
  std::vector<Domino> dominos;
  for (const std::string& line : domino_lines) {
    const std::vector<std::string> words = Split(line, ' ');
    for (const std::string& word : words) {
      if (word.size() != 2) {
	die("Invalid domino: " + word);
      }
      const Value a = ParseValue(word.substr(0, 1));
      const Value b = ParseValue(word.substr(1, 1));
      if (a < 0 || a > 6 || b < 0 || b > 6) {
	die("Invalid domino: " + word);
      }
      const DominoId id = dominos.size();
      dominos.push_back({
	  .id = id,
	  .lower_value = std::min(a, b),
	  .upper_value = std::max(a, b),
	});
    }
  }
  return dominos;
}

Puzzle ReadPuzzle() {
  std::cout << "Enter the grid." << std::endl;
  std::cout << " - 0 for empty cell" << std::endl;
  std::cout << " - space for no cell" << std::endl;
  std::cout << " - letters for constraint groups" << std::endl;
  std::cout << " - empty line to end" << std::endl;
  const std::vector<std::string> grid_lines = ReadNonEmptyLines();

  std::cout << "Enter the constraints." << std::endl;
  std::cout << " - for example, A:6 or B:<5 or C:= or D:!=" << std::endl;
  std::cout << " - one constraint per line" << std::endl;
  std::cout << " - empty line to end" << std::endl;
  const std::vector<std::string> constraint_lines = ReadNonEmptyLines();

  std::cout << "Enter the dominos." << std::endl;
  std::cout << " - for example, 11 or 65 or 23 or 60" << std::endl;
  std::cout << " - separated by spaces, as many per line as desired" << std::endl;
  std::cout << " - empty line to end" << std::endl;
  const std::vector<std::string> domino_lines = ReadNonEmptyLines();

  // Parse the constraints first.
  ConstraintMapping constraint_mapping = ParseConstraints(constraint_lines);

  // Figure out how big the grid needs to be.
  int rows = grid_lines.size();
  int cols = 0;
  for (const std::string& row : grid_lines) {
    cols = std::max(cols, static_cast<int>(row.size()));
  }
  Grid<std::optional<CellId>> grid(rows, cols);

  // Parse the grid.
  CellId next_cell_id = 0;
  for (int r = 0; r < rows; ++r) {
    const std::string& row_line = grid_lines[r];
    for (int c = 0; c < row_line.size(); ++c) {
      const char value = row_line[c];
      if (value == ' ') {
	// Nothing to do!
      } else if (value == '0') {
	grid.Set(r, c, next_cell_id++);
      } else {
	CellId cell_id = next_cell_id++;
	grid.Set(r, c, cell_id);
	auto it = constraint_mapping.name_to_id.find(value);
	if (it == constraint_mapping.name_to_id.end()) {
	  die("Invalid constraint name in grid: " + value);
	}
	ConstraintId constraint_id = it->second;
	constraint_mapping.constraints[constraint_id].cells.push_back(cell_id);
      }
    }
  }

  // Parse the dominos.
  std::vector<Domino> dominos = ParseDominos(domino_lines);

  return Puzzle{
    .grid = std::move(grid),
    .dominos = std::move(dominos),
    .constraints = std::move(constraint_mapping.constraints),
  };
}

struct CheckConstraintVisitor {
public:
  explicit CheckConstraintVisitor(const Constraint& constraint,
				  const std::vector<std::optional<Value>>& cell_to_value)
    : constraint(constraint),
      cell_to_value(cell_to_value) {}
  
  bool operator()(const Identical& all_equal) const {
    std::optional<Value> constraint_value;
    for (const CellId cell : constraint.cells) {
      const std::optional<Value> cell_value = cell_to_value[cell];
      if (cell_value.has_value() && constraint_value.has_value() &&
	  *cell_value != *constraint_value) {
	return false;
      }
      if (cell_value.has_value()) {
	constraint_value = cell_value;
      }
    }
    return true;
  }
  bool operator()(const Distinct& distinct) const {
    std::array<bool, 6> seen = {false, false, false, false, false, false};
    for (const CellId cell : constraint.cells) {
      const std::optional<Value> cell_value = cell_to_value[cell];
      if (!cell_value.has_value()) {
	continue;
      }
      if (seen[*cell_value]) {
	return false;
      }
      seen[*cell_value] = true;
    }
    return true;
  }
  bool operator()(const SumEqualTo& equal_to) const {
    return true;
  }
  bool operator()(const SumLessThan& less_than) const {
    return true;
  }
  bool operator()(const SumGreaterThan& greater_than) const {
    return true;
  }

private:
  const Constraint& constraint;
  const std::vector<std::optional<Value>>& cell_to_value;
};

bool CheckConstraint(const Constraint& constraint,
		     const std::vector<std::optional<Value>>& cell_to_value) {
  CheckConstraintVisitor visitor(constraint, cell_to_value);
  return std::visit(visitor, constraint.type);
}

/*

  using LocalConstraintState = std::variant<IdenticalLocalState, DistinctLocalState, SumEqualToLocalState, ...>;

  Local constraints have a Set(cell_id, value) that updates the internal state and returns bool of if satisfied or not.

  To assess placement of a piece from local constraints:
   - Find the constraint for each cell
   - For the first, make a copy of the local constraint state, do Set(), bail if invalid
   - For the second, if same constraint use the same local state otherwise make a copy of the other local constraint state, do Set(), bail if invalid
   
  To assess placement of a piece from global constraints:
   - Each gobal constraint state also has a Set() that updates internal constraint state and returns a Delta
   - Similarly apply the constrains to copies, accumulate up to two Deltas
   - Get new global constraint set by applying deltas (to a copy)
   - Get new global available pips (in a copy)
   - Validate

  Global constraint is:
    - min pips
    - max pips
    - min 1s
    - min 2s
    - min 3s
    - min 4s
    - min 5s
    - min 6s

*/

class AnalyzedPuzzle {
private:
  Puzzle puzzle_;
  std::vector<std::optional<ConstraintId>> cell_to_constraint_;
};

int CountCells(const Grid<std::optional<CellId>>& grid) {
  int count = 0;
  for (int r = 0; r < grid.num_rows(); ++r) {
    for (int c = 0; c < grid.num_cols(); ++c) {
      const std::optional<CellId> cell = grid.Get(r, c);
      if (cell.has_value()) {
	++count;
      }
    }
  }
  return count;
}

class PositionGraph {
public:
  PositionGraph(const Puzzle& puzzle) : grid_(puzzle.grid) {
    // Set up the graph topology.
    num_cells_ = CountCells(grid_);
    cell_is_placed_.resize(num_cells_, false);
    cell_is_even_.resize(num_cells_, false);
    neighbors_.resize(num_cells_);
    edge_states_.resize(num_cells_ * num_cells_, DELETED);
    for (int r = 0; r < grid_.num_rows(); ++r) {
      for (int c = 0; c < grid_.num_cols(); ++c) {
	const std::optional<CellId> cell = grid_.Get(r, c);
	if (!cell.has_value()) {
	  continue;
	}
	cell_is_even_[*cell] = (r + c) % 2 == 0;
	for (const auto& [dr, dc] : kNeighborOffsets) {
	  if (!grid_.Valid(r + dr, c + dc)) {
	    continue;
	  }
	  const std::optional<CellId> neighbor = grid_.Get(r + dr, c + dc);
	  if (!neighbor.has_value()) {
	    continue;
	  }
	  if (GetEdgeState(*cell, *neighbor) != DELETED) {
	    continue;
	  }
	  neighbors_[*cell].push_back(*neighbor);
	  neighbors_[*neighbor].push_back(*cell);
	  SetEdgeState(*cell, *neighbor, UNMATCHED);
	}
      }
    }

    // for (CellId cell = 0; cell < num_cells_; ++cell) {
    //   std::cout << static_cast<int>(cell) << ": ";
    //   for (const auto& n : neighbors_[cell]) {
    // 	std::cout << static_cast<int>(n) << ", ";
    //   }
    //   std::cout << std::endl;
    // }

    // Find a valid matching.
    Solve();
  }
  
  void PlacePiece(const Position& position) {
    // TODO remove this eventually.
    if (IsPlaced(position.lower_cell) || IsPlaced(position.upper_cell)) {
      die("Can't place piece over cells that are already placed");
    }
    if (IsUnmatched(position.lower_cell) || IsUnmatched(position.upper_cell)) {
      die("Internal error: PositionGraph is in invalid unmatched state");
    }

    // If this position is not currently matched, then after placing the piece
    // we will need to repair our matching. We should start our repair from one
    // of the cells that is currently matched to one of the position's cells.
    std::optional<CellId> repair_starting_point;
    if (GetEdgeState(position.lower_cell, position.upper_cell) == UNMATCHED) {
      for (const CellId neighbor : neighbors_[position.lower_cell]) {
	if (GetEdgeState(position.lower_cell, neighbor) == MATCHED) {
	  repair_starting_point = neighbor;
	  break;
	}
      }
      if (!repair_starting_point.has_value()) {
	die("Internal error: PositionGraph is in invalid unmatched state");
      }
    }

    // Delete all edges around the placed piece.
    for (const CellId cell : {position.lower_cell, position.upper_cell}) {
      for (const CellId neighbor : neighbors_[cell]) {
	SetEdgeState(cell, neighbor, DELETED);
      }
    }

    // Mark the cells placed.
    cell_is_placed_[position.lower_cell] = true;
    cell_is_placed_[position.upper_cell] = true;

    // If we need to repair, do that by finding an augmenting path.
    if (repair_starting_point.has_value()) {
      if (!FindAndFlipAugmentingPathStartingFrom(*repair_starting_point)) {
	die("Internal error: cannot repair PositionGraph after placement");
      }
    }
  }

  void UnplacePiece(const Position& position) {
    // TODO remove this eventually.
    if (!IsPlaced(position.lower_cell) || !IsPlaced(position.upper_cell)) {
      die("Can't unplace piece over cells that are not placed");
    }

    // Mark the cells un-placed.
    cell_is_placed_[position.lower_cell] = false;
    cell_is_placed_[position.upper_cell] = false;

    // Un-delete all the edges to non-placed cells around the piece.
    for (const CellId cell : {position.lower_cell, position.upper_cell}) {
      for (const CellId neighbor : neighbors_[cell]) {
	if (!IsPlaced(neighbor)) {
	  SetEdgeState(cell, neighbor, UNMATCHED);
	}
      }
    }

    // We expect there to be an unmatched edge between the two cells in the
    // removed piece. If there is, flip it to matched. If there is not,
    // something has gone wrong.
    if (GetEdgeState(position.lower_cell, position.upper_cell) != UNMATCHED) {
      die("Internal error: cannot unplace piece where the two cells don't have an edge");
    }

    SetEdgeState(position.lower_cell, position.upper_cell, MATCHED);
  }

  // std::vector<Position> AllAllowedPositions() const {
  //   std::unordered_map<CellId, int> visited_cell_depth;
  //   std::vector<Position> valid_positions = CurrentMatchingPositions();

  //   for (CellId cell = 0; cell < num_cells_; ++cell) {
  //     std::cout << "Starting search from " << static_cast<int>(cell) << std::endl;
  //     const int starting_depth = 1000 * (num_cells_ - cell);
  //     AllAllowedPositionsHelper(cell, starting_depth, /*desired_next_edge=*/MATCHED,
  // 				visited_cell_depth, valid_positions);
  //   }

  //   return valid_positions;
  // }

  std::vector<Position> AllAllowedPositions() const {
    // Use Tarjan's algorithm to find cycles that alternate between matched and
    // unmatched edges. Each edge that is part of such a cycle corresponds to an
    // allowed position.
    TarjanState state;
    state.nodes.resize(num_cells_);
    state.stack.reserve(num_cells_);

    for (CellId cell = 0; cell < num_cells_; ++cell) {
      if (!state.nodes[cell].index.has_value()) {
	// Since we want to find only cycles that alternate matched and
	// unmatched edges, and since the graph is bipartite, we can accomplish
	// this by virtually constructing a directed graph where all the matched
	// edges go from left to right (from the even cells to the odd cells)
	// and all the unmatched edges go from right to left (from the odd cells
	// to the even cells). We can then reduce the problem to finding *any*
	// cycles (without restrictions) in this virtual graph. This reduction
	// demonstrates that Tarjan's algorithm will be correct here. We don't
	// explicitly construct this directed graph, though, we just keep track
	// of what edge type we're currently on and alternate as we traverse. To
	// ensure that we make consistent decisions when assigning a direction
	// to an edge, we use the parity of the starting cell to determine the
	// first edge type to search for.
	EdgeState desired_next_edge = cell_is_even_[cell] ? MATCHED : UNMATCHED;
	AllAllowedPositionsHelper(state, cell, desired_next_edge);
      }
    }

    // Allowed positions are all those edges that are either already in the
    // current matching, or where both nodes are in the same SCC.
    std::vector<Position> allowed_positions;
    for (CellId cell = 0; cell < num_cells_; ++cell) {
      for (CellId neighbor : neighbors_[cell]) {
	if (cell >= neighbor) {
	  // Positions are unordered pairs.
	  continue;
	}
	const EdgeState edge_state = GetEdgeState(cell, neighbor);
	if (edge_state == DELETED) {
	  continue;
	}
	if (edge_state == MATCHED || 
	    state.nodes[cell].scc_index == state.nodes[neighbor].scc_index) {
	  allowed_positions.push_back({
	      .lower_cell = cell,
	      .upper_cell = neighbor,
	    });
	}
      }
    }
    return allowed_positions;
  }

  Grid<char> MakePrintGridForPositions(const std::vector<Position>& positions) {
    std::unordered_map<CellId, std::pair<int, int>> cell_to_rc;
    Grid<char> print_grid(grid_.num_rows() * 4, grid_.num_cols() * 5, ' ');
    for (int r = 0; r < grid_.num_rows(); ++r) {
      for (int c = 0; c < grid_.num_cols(); ++c) {
	const std::optional<CellId> cell = grid_.Get(r, c);
	if (!cell.has_value()) {
	  continue;
	}
	cell_to_rc[*cell] = std::make_pair(r, c);
	print_grid.Set(r * 4,     c * 5,     '+');
	print_grid.Set(r * 4,     c * 5 + 1, '-');
	print_grid.Set(r * 4,     c * 5 + 2, '-');
	print_grid.Set(r * 4,     c * 5 + 3, '-');
	print_grid.Set(r * 4,     c * 5 + 4, '+');
	print_grid.Set(r * 4 + 1, c * 5,     '|');
	print_grid.Set(r * 4 + 1, c * 5 + 4, '|');
	print_grid.Set(r * 4 + 2, c * 5,     '|');
	print_grid.Set(r * 4 + 2, c * 5 + 4, '|');
	print_grid.Set(r * 4 + 3, c * 5,     '+');
	print_grid.Set(r * 4 + 3, c * 5 + 1, '-');
	print_grid.Set(r * 4 + 3, c * 5 + 2, '-');
	print_grid.Set(r * 4 + 3, c * 5 + 3, '-');
	print_grid.Set(r * 4 + 3, c * 5 + 4, '+');
	if (IsPlaced(*cell)) {
	  print_grid.Set(r * 4 + 1, c * 5 + 1, 'X');
	  print_grid.Set(r * 4 + 1, c * 5 + 2, 'X');
	  print_grid.Set(r * 4 + 1, c * 5 + 3, 'X');
	  print_grid.Set(r * 4 + 2, c * 5 + 1, 'X');
	  print_grid.Set(r * 4 + 2, c * 5 + 2, 'X');
	  print_grid.Set(r * 4 + 2, c * 5 + 3, 'X');
	}
      }
    }

    for (const Position& position : positions) {
      const auto& [r1, c1] = cell_to_rc[position.lower_cell];
      const auto& [r2, c2] = cell_to_rc[position.upper_cell];
      const int dr = r2 - r1;
      const int dc = c2 - c1;
      if (dr == 1 && dc == 0) {
	print_grid.Set(r1 * 4 + 2, c1 * 5 + 2, '*');
	print_grid.Set(r1 * 4 + 3, c1 * 5 + 2, '*');
	print_grid.Set(r1 * 4 + 4, c1 * 5 + 2, '*');
	print_grid.Set(r1 * 4 + 5, c1 * 5 + 2, '*');
      } else if (dr == -1 && dc == 0) {
	print_grid.Set(r2 * 4 + 2, c1 * 5 + 2, '*');
	print_grid.Set(r2 * 4 + 3, c1 * 5 + 2, '*');
	print_grid.Set(r2 * 4 + 4, c1 * 5 + 2, '*');
	print_grid.Set(r2 * 4 + 5, c1 * 5 + 2, '*');
      } else if (dr == 0 && dc == 1) {
	print_grid.Set(r1 * 4 + 1, c1 * 5 + 3, '_');
	print_grid.Set(r1 * 4 + 1, c1 * 5 + 4, '_');
	print_grid.Set(r1 * 4 + 1, c1 * 5 + 5, '_');
	print_grid.Set(r1 * 4 + 1, c1 * 5 + 6, '_');
      } else if (dr == 0 && dc == -1) {
	print_grid.Set(r1 * 4 + 1, c2 * 5 + 3, '_');
	print_grid.Set(r1 * 4 + 1, c2 * 5 + 4, '_');
	print_grid.Set(r1 * 4 + 1, c2 * 5 + 5, '_');
	print_grid.Set(r1 * 4 + 1, c2 * 5 + 6, '_');
      } else {
	die("Invalid connection after soliving");
      }
    }

    return print_grid;
  }
  
  void DebugPrint() {
    const Grid<char> current_grid =
      MakePrintGridForPositions(CurrentMatchingPositions());

    const Grid<char> all_valid_grid =
      MakePrintGridForPositions(AllAllowedPositions());
    
    PrintCharGridsLeftToRight({current_grid, all_valid_grid});
  }
  
private:
  struct TarjanNode {
    std::optional<int> index;
    std::optional<int> low_link;
    std::optional<int> scc_index;
    bool on_stack = false;
  };

  struct TarjanState {
    std::vector<CellId> stack;
    std::vector<TarjanNode> nodes;
    int next_index = 0;
    int next_scc_index = 0;
  };
  
  static constexpr std::array kNeighborOffsets = {
    std::make_pair(1, 0),
    std::make_pair(-1, 0),
    std::make_pair(0, 1),
    std::make_pair(0, -1),
  };
  
  enum EdgeState {
    DELETED = 0,
    UNMATCHED = 1,
    MATCHED = 2,
  };

  static EdgeState Flip(EdgeState edge_state) {
    switch (edge_state) {
    case UNMATCHED:
      return MATCHED;
    case MATCHED:
      return UNMATCHED;
    case DELETED:
    default:
      return DELETED;
    }
  }

  EdgeState GetEdgeState(CellId a, CellId b) const {
    const CellId i = std::min(a, b);
    const CellId j = std::max(a, b);
    return edge_states_[i * num_cells_ + j];
  }

  void SetEdgeState(CellId a, CellId b, EdgeState new_state) {
    const CellId i = std::min(a, b);
    const CellId j = std::max(a, b);
    edge_states_[i * num_cells_ + j] = new_state;
  }

  void FlipEdgeState(CellId a, CellId b) {
    SetEdgeState(a, b, Flip(GetEdgeState(a, b)));
  }

  bool IsUnmatched(CellId cell) const {
    // An unmatched cell has at least one unmatched edge, and no matched edges.
    bool has_unmatched = false;
    for (const CellId neighbor : neighbors_[cell]) {
      switch (GetEdgeState(cell, neighbor)) {
      case UNMATCHED:
	has_unmatched = true;
	continue;
      case MATCHED:
	return false;
      }
    }
    return has_unmatched;
  }

  bool IsPlaced(CellId cell) const {
    return cell_is_placed_[cell];
  }

  std::vector<Position> CurrentMatchingPositions() const {
    std::vector<Position> current_matching_positions;
    for (CellId cell = 0; cell < num_cells_; ++cell) {
      for (CellId neighbor : neighbors_[cell]) {
	if (GetEdgeState(cell, neighbor) == MATCHED) {
	  current_matching_positions.push_back({
	      .lower_cell = std::min(cell, neighbor),
	      .upper_cell = std::max(cell, neighbor),
	    });
	}
      }
    }
    return current_matching_positions;
  }

  void AllAllowedPositionsHelper(TarjanState& state, CellId cell, EdgeState desired_next_edge) const {
    TarjanNode& node = state.nodes[cell];
    node.index = ++state.next_index;
    node.low_link = *node.index;
    node.on_stack = true;

    state.stack.push_back(cell);

    for (const CellId neighbor : neighbors_[cell]) {
      // Skip edges of the wrong type.
      if (GetEdgeState(cell, neighbor) != desired_next_edge) {
	continue;
      }

      TarjanNode& neighbor_node = state.nodes[neighbor];

      if (!neighbor_node.index.has_value()) {
	// This node has not be visited yet; recurse into it, and this edge
	// becomes part of the spanning forest.
	AllAllowedPositionsHelper(state, neighbor, Flip(desired_next_edge));
	// Check if we found a path to an earlier node.
	node.low_link = std::min(*node.low_link, *neighbor_node.low_link);
      } else if (neighbor_node.on_stack) {
	// This node is already on the stack; that means this edge is a back
	// edge (not in the spanning forest), so no need to traverse but we do
	// need to update the low_link.
	node.low_link = std::min(*node.low_link, *neighbor_node.index);
      } else {
	// Don't explore this edge.
      }
    }

    // Check if this node is the root of a SCC.
    if (*node.low_link == *node.index) {
      // Iteratively pop the stack to get all members of this SCC.
      CellId scc_cell;
      do {
	scc_cell = state.stack.back();
	state.stack.pop_back();
	TarjanNode& scc_node = state.nodes[scc_cell];
	scc_node.on_stack = false;
	scc_node.scc_index = state.next_scc_index;
      } while (scc_cell != cell);
      ++state.next_scc_index;
    }
  }

  
  // int AllAllowedPositionsHelper(CellId cell, int depth, EdgeState desired_next_edge,
  // 				std::unordered_map<CellId, int>& visited_cell_depth,
  // 				std::vector<Position>& valid_positions) const {
  //   for (int i = 0; i < depth % 1000; ++i) {
  //     std::cout << "    ";
  //   }
  //   std::cout << "Visited " << static_cast<int>(cell) << " at depth " << depth << std::endl;
    
  //   // If we've already visited this cell, we've found a cycle. Return the depth
  //   // of the node that begins the cycle.
  //   const auto it = visited_cell_depth.find(cell);
  //   if (it != visited_cell_depth.end()) {
  //     for (int i = 0; i < depth % 1000; ++i) {
  // 	std::cout << "    ";
  //     }
  //     std::cout << "  Already seen at depth " << it->second << std::endl;
  //     return it->second;
  //   }

  //   // Otherwise mark this node as visited and continue DFS. Record the min
  //   // observed cycle depth along any cycle we discover.
  //   visited_cell_depth[cell] = depth;
  //   int min_cycle_depth = std::numeric_limits<int>::max();
  //   for (const CellId neighbor : neighbors_[cell]) {
  //     if (GetEdgeState(cell, neighbor) != desired_next_edge) {
  // 	// Skip edges of the wrong type.
  // 	continue;
  //     }

  //     const int cycle_depth =
  // 	AllAllowedPositionsHelper(neighbor, depth + 1, Flip(desired_next_edge),
  // 				  visited_cell_depth, valid_positions);
  //     min_cycle_depth = std::min(min_cycle_depth, cycle_depth);

  //     // If we found a cycle that reaches back to this node or some precedessor,
  //     // then this edge is along a cycle and is a valid position.
  //     if (cycle_depth <= depth) {
  // 	for (int i = 0; i < depth % 1000; ++i) {
  // 	  std::cout << "    ";
  // 	}
  // 	std::cout << "  Found valid position " << static_cast<int>(cell) << " to " << static_cast<int>(neighbor) << std::endl;
  // 	valid_positions.push_back({
  // 	    .lower_cell = std::min(cell, neighbor),
  // 	    .upper_cell = std::max(cell, neighbor),
  // 	  });
  //     }
  //   }

  //   // Update cell depth to be the min of what was observed, and communicate
  //   // min_cycle_depth up to predecessors so they know whether or not they are
  //   // part of at least one cycle that we found.
  //   for (int i = 0; i < depth % 1000; ++i) {
  //     std::cout << "    ";
  //   }
  //   std::cout << "  Min depth was " << min_cycle_depth << std::endl;
  //   visited_cell_depth[cell] = std::numeric_limits<int>::max();
  //   return min_cycle_depth;
  // }

  bool FindAndFlipAugmentingPathHelper(CellId cell,
				       std::unordered_set<CellId>& visited,
				       EdgeState desired_next_edge) {
    if (desired_next_edge == MATCHED && IsUnmatched(cell)) {
      // We've successfully terminated an augmenting path if we just followed an
      // unmatched edge to an unmatched node.
      return true;
    }

    // Don't revisit cells.
    if (visited.count(cell) != 0) {
      return false;
    }
    visited.insert(cell);

    // Try to find an edge of the correct type to follow.
    for (const CellId neighbor : neighbors_[cell]) {
      if (GetEdgeState(cell, neighbor) == desired_next_edge) {
	bool found_path = FindAndFlipAugmentingPathHelper(neighbor,
							  visited,
							  Flip(desired_next_edge));
	if (found_path) {
	  // We found an augmenting path! Return, and as we unwind the stack
	  // also flip all the edge state.
	  FlipEdgeState(cell, neighbor);
	  return true;
	}
	// Otherwise, try the next edge.
      }
    }

    // No augmenting path found.
    return false;
  }

  bool FindAndFlipAugmentingPathStartingFrom(CellId start) {
    if (!IsUnmatched(start)) {
      return false;
    }
    std::unordered_set<CellId> visited;
    return FindAndFlipAugmentingPathHelper(start, visited, UNMATCHED);
  }

  bool FindAndFlipAugmentingPath() {
    for (CellId cell = 0; cell < num_cells_; ++cell) {
      if (FindAndFlipAugmentingPathStartingFrom(cell)) {
	return true;
      }
    }
    return false;
  }
  
  void Solve() {
    int matching_size = 0;
    while (FindAndFlipAugmentingPath()) {
      matching_size += 2;
    }
    if (matching_size != num_cells_) {
      die("Grid cannot be tiled with dominos");
    }
  }
  
  // Only needed for debug printing.
  Grid<std::optional<CellId>> grid_;
  
  int num_cells_ = 0;

  // neighbors_[i] is a list of the CellIds that are connected to the cell with
  // CellId = i.
  std::vector<std::vector<CellId>> neighbors_;

  // Given an edge (i, j), where i < j, the state of the edge is in
  // edge_states_[i * num_cells_ + j]. Entries that don't correspond to a valid
  // edge will be DELETED.
  std::vector<EdgeState> edge_states_;

  std::vector<bool> cell_is_placed_;

  // An "even" cell is one where row + col is even. If these cells were colored
  // with a checkerboard pattern, these would be the black cells. Since every
  // edge must connect an even cell to an odd cell, this demonstrates that the
  // graph is bipartite.
  std::vector<bool> cell_is_even_;
};
					     

class BoardState {
  
private:
  void MakeMove(const Move move);
  void Backtrack();
  
  AnalyzedPuzzle puzzle_;

  struct Step {
    Move move;

    // Backtracking info.
    std::vector<Move> culled_moves;
  };

  std::vector<Step> steps_;
  PositionGraph position_graph_;
  std::vector<Move> allowed_next_moves_;
  std::vector<std::optional<int>> cell_to_value_;
};


// Check that move is at least kinda legal:
//   - that position must be allowed by position graph
//   - that position must not directly violate any constraints
//   - it must still be possible to satisfy constraints after the placement



int main(int argc, char* argv[]) {
  // std::variant<int, float, std::string> v = "Hello";
  // std::visit([](auto&& arg) { std::cout << arg; }, v);
  
  Puzzle puzzle = ReadPuzzle();  
  puzzle.DebugPrint();

  PositionGraph graph(puzzle);
  graph.DebugPrint();

  std::cout << "Enter command to place/unplace, like p 0 1 or u 2 3" << std::endl;
  std::cout << "Empty line to quit" << std::endl;
  std::string line;
  std::getline(std::cin, line);
  for (; !line.empty(); std::getline(std::cin, line)) {
    const std::vector<std::string> parts = Split(line, ' ');
    if (parts.size() != 3) {
      std::cerr << "Wrong number of arugments" << std::endl;
      continue;
    }
    std::string command = parts[0];
      CellId a = ParseValue(parts[1]);
      CellId b = ParseValue(parts[2]);
    if (command == "p") {
      graph.PlacePiece({.lower_cell = a, .upper_cell = b});
      graph.DebugPrint();
    } else if (command == "u") {
      graph.UnplacePiece({.lower_cell = a, .upper_cell = b});
      graph.DebugPrint();
    } else {
      std::cerr << "Unrecognized command: " << command << std::endl;
      continue;
    }
  }

  return 0;
}
