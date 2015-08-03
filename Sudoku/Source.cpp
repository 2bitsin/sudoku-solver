#include <cstdint>
#include <iostream>
#include <string>
#include <array>
#include <memory>
#include <algorithm>

static const std::uint32_t mapping [9u] [9u] = {
    {0, 1, 2, 9, 10, 11, 18, 19, 20},
    {3, 4, 5, 12, 13, 14, 21, 22, 23},
    {6, 7, 8, 15, 16, 17, 24, 25, 26},
    {27, 28, 29, 36, 37, 38, 45, 46, 47},
    {30, 31, 32, 39, 40, 41, 48, 49, 50},
    {33, 34, 35, 42, 43, 44, 51, 52, 53},
    {54, 55, 56, 63, 64, 65, 72, 73, 74},
    {57, 58, 59, 66, 67, 68, 75, 76, 77},
    {60, 61, 62, 69, 70, 71, 78, 79, 80}
};

struct grid {
    typedef std::uint8_t value_type;
    typedef std::uint16_t meta_type;
    typedef std::array<value_type, 81u> value_grid_type;
    typedef std::array<meta_type, 81u> meta_grid_type;

    grid (): 
        value {
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        meta {
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0
        }
    {}
    grid (std::istream& in): grid () {
        in >> *this;
    }

    grid (const grid&) = default;
    grid& operator = (const grid&) = default;

    friend std::istream& operator >> (std::istream& in, grid& grid) {
        std::string line;
        std::getline (in, line);
        if (line.size () != 81u)
            throw std::runtime_error ("Invalid input");        
        std::transform (
            line.begin (), line.end (), grid.value.begin (), 
            [] (auto c) { return c != '.' ? (c - '0') : 0 ; }
        );
        return in;
    }

    friend std::ostream& operator << (std::ostream& out, const grid& g) {
        for (auto y = 0; y < 3u; ++y) {
            if (y != 0) std::cout << " ------+-------+------\n";
            for (auto i = 0; i < 3u; ++i) {                
                for (auto x = 0; x < 3u; ++x) {                    
                    if (x != 0) std::cout << " |";
                    for (auto j = 0; j < 3u; ++j) {
                        auto& v = g.value [(y*3 + i)*9 + (x*3 + j)];
                        if (v != 0) std::cout << " " << int (v);
                        else std::cout << " .";
                    }
                }
                std::cout << "\n";
            }
        }
        return out;
    }

    value_grid_type value;
    meta_grid_type meta;    
};

template <typename _Ctype>
auto collect (const _Ctype& in) {
    std::uint16_t mask = 0u;
    for (auto i = 0u; i < 9u; ++i) 
        mask |= (1u << in (i));    
    return mask >> 1u;
}

auto collect_row (const grid& g, std::uint32_t row) {
    auto& value = g.value;
    return collect ([&value, row] (auto i) {
        return value [i + row*9u];
    });
}

auto collect_col (const grid& g, std::uint32_t col) {
    auto& value = g.value;
    return collect ([&value, col] (auto i) {
        return value [i*9u + col];
    });
}

auto collect_cel (const grid& g, std::uint32_t idx) {
    auto& value = g.value;
    return collect ([&value, idx] (auto i) {
        return value [mapping [idx][i]];
    });
}

auto prune_all (grid& g) {
    std::uint16_t rows [9u], cols [9u], cels [9u];
    for (auto i = 0u; i < 9u; ++i) {
        rows [i] = collect_row (g, i);
        cols [i] = collect_col (g, i);
        cels [i] = collect_cel (g, i);
    }

    for (auto j = 0u; j < 9u; ++j) {
        for (auto i = 0u; i < 9u; ++i) {
            auto k = (j/3u)*3u + (i/3u);
            g.meta [j*9u + i] = cels [k]|rows [j]|cols [i];            
        }
    }
}

template <typename _Ctype>
auto for_each_available (std::uint16_t m, const _Ctype& call) {
    for (auto j = 0u; j < 9; ++j) {
        if (m & (1u << j))
            continue;
        call (j + 1u);
    }
}

auto mutate (const grid& g, std::uint32_t x, std::uint32_t y, std::uint16_t v) {
    auto new_g = std::make_unique<grid> (g);    
    auto &ng = *new_g;
    auto flag = 0u;

    ng.value [x + y*9u] = grid::value_type (v);

    auto k = (y/3u)*3u + (x/3u);

    auto mr = collect_row (ng, y);
    auto mc = collect_col (ng, x);
    auto me = collect_cel (ng, k);

    for (auto i = 0u; i < 9u; ++i) {
        flag += ((ng.meta [i*9+x] |= mr) == 0x3FF);
        flag += ((ng.meta [y*9+i] |= mc) == 0x3FF);             
        flag += ((ng.meta [mapping [k] [i]] |= me) == 0x3FF);            
    }

    return flag;
}



int main (int, char**) try {
    auto root = std::make_unique<grid> (std::cin);     
    prune_all (*root);
    std::cout << *root;    


    return 0;
}
catch (const std::exception& e) {
    std::cout << e.what () << "\n";
    return -1;
}