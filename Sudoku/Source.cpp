#include <iostream>
#include <string>
#include <array>
#include <cstdint>
#include <algorithm>

struct puzzle {
    typedef std::array<std::uint16_t, 81u> grid_t;

    template <typename... _Vtype>
    puzzle (_Vtype... in) : 
        grid {static_cast<grid_t::value_type> (in & 0xf)...}
    {}
        
    puzzle (): puzzle (
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0) 
    {};
    
    friend std::istream& operator >> (std::istream& in, puzzle& p) {
        using namespace std::string_literals;        
        grid_t new_grid;

        std::string line;
        std::getline (in, line);
        if (line.size () != 81u) {
            throw std::runtime_error ("Invalid input");
        }

        std::transform (
            line.begin (), line.end (), 
            new_grid.begin (), [] (auto&& c) {            
                return std::uint16_t (c == '.' ? 0u : c - '0') & 0xf;
            });

        return in;
    }
    
    void reset (const grid_t& g) {
        grid = g;
    }

protected:
    void prune () {
        for (auto y = 0u; y < 9u; ++y) {
            std::uint16_t mask = 0u;
            for (auto x = 0u; x < 9u; ++x) {
                mask |= (1 << (grid [x + y*9u] & 0xf));
            }
            mask = (mask >> 1) << 3;
            for (auto x = 0u; x < 9u; ++x) {
                if (grid [x + y*9u] & 0xf) {
                    grid [x + y*9u] |= 0xfff;
                    continue;
                }
                grid [x + y*9u] |= mask;
            }
        }
        for (auto x = 0u; x < 9u; ++x) {
            std::uint16_t mask = 0u;
            for (auto y = 0u; y < 9u; ++y) {
                mask |= (1 << (grid [x + y*9u] & 0xf));
            }
            mask = (mask >> 1) << 3;
            for (auto y = 0u; y < 9u; ++y) {
                if (grid [x + y*9u] & 0xf)
                    continue;
                grid [x + y*9u] |= mask;
            }
        }
    }

private:
    grid_t grid;
};


int main (int, char**) {
    puzzle (1, 2, 3, 4);
}