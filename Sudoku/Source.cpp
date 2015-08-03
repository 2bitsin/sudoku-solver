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
        p.reset (new_grid);
        return in;
    }
    
    void reset (const grid_t& g) {
        grid = g;
        prune ();
    }

    const auto& cell (std::uint32_t x, std::uint32_t y) const {
        return grid [x + y * 9u];
    }

    auto& cell (std::uint32_t x, std::uint32_t y) {
        return grid [x + y * 9u];
    }
    
    auto cell_value (std::uint32_t x, std::uint32_t y) const {
        return cell (x, y) & 0xf;
    }

    auto cell_mask (std::uint32_t x, std::uint32_t y) const {
        return cell (x, y) >> 4u;
    }

    void set_value (std::uint32_t x, std::uint32_t y, std::uint8_t v) {
        cell (x, y) = (cell (x, y) & 0xfff0) | (v & 0xf);
    }

    void set_mask (std::uint32_t x, std::uint32_t y, std::uint16_t m) {
        cell (x, y) = (cell (x, y) & 0xf) | (m << 4);
    }

    void mutate (std::uint32_t x, std::uint32_t y) {

    }

protected:    

    std::uint16_t calculate_row_set (std::uint32_t row) {
        std::uint16_t mask = 0;
        for (auto i = 0u; i < 9u; ++i)
            mask |= (1u << cell_value (i, row)); 
        return mask >> 1u; 
    }

    std::uint16_t calculate_col_set (std::uint32_t col) {
        std::uint16_t mask = 0;
        for (auto i = 0u; i < 9u; ++i)
            mask |= (1u << cell_value (col, i));
        return mask >> 1u; 
    }

    std::uint16_t calculate_3x3_set (std::uint32_t x, std::uint32_t y) {
        std::uint16_t mask = 0;
        for (auto i = 0u; i < 3u; ++i) {
            for (auto j = 0u; j < 3u; ++j) {
                mask |= (1u << cell_value (x*3u + i, y*3u + j));
            }
        }
        return mask >> 1u;
    }

    void prune () {
        for (auto y = 0u; y < 9u; ++y) {
            auto mask = calculate_row_set (y);
            for (auto x = 0u; x < 9u; ++x)
                set_mask (x, y, mask);            
        }
        for (auto x = 0u; x < 9u; ++x) {
            auto mask = calculate_col_set (x);
            for (auto y = 0u; y < 9u; ++y)
                set_mask (x, y, mask);
        }
        for (auto x = 0u; x < 3u; ++x) {
            for (auto y = 0u; y < 3u; ++y) {
                auto mask = calculate_3x3_set (x, y);
                for (auto i = 0u; i < 3u; ++i) {
                    for (auto j = 0u; j < 3u; ++j) {
                        set_mask (x*3u + i, y*3u + j, mask);
                    }
                }
            }
        }
    }

private:
    grid_t grid;
};


int main (int, char**) try {
    puzzle p;
    std::cin >> p;
    return 0;
}
catch (const std::exception& e) {
    std::cout << e.what () << "\n";
}