//
//  Data_structs.cpp
//  SFML Chess
//
//  Created by Andrew Xia on 4/16/21.
//  Copyright © 2021 Andy. All rights reserved.
//

#include "Data_structs.hpp"

bool Cords::operator==(const Cords c2) {
    return (this->x == c2.x && this->y == c2.y);
}
bool Cords::operator!=(const Cords c2) {
    return !(*this == c2);
}
Cords::Cords() {
    x = -1;
    y = -1;
}
Cords::Cords(int a, int b) {
    x = a; y = b;
}

 
namespace std
{
    size_t hash<Cords>::operator()(Cords const& c) const noexcept
    {
        size_t h1 = std::hash<int>{}(c.x);
        size_t h2 = std::hash<int>{}(c.y);
        size_t seed = 42;
        seed ^= h1 + 0x9e3779b97f4a7c15 + (seed << 6) + (seed >> 2);
        seed ^= h2 + 0x9e3779b97f4a7c15 + (seed << 6) + (seed >> 2);; // or use h1 ^ (h2 << 1)
        return seed;
    }
}

bool cords_eq::operator()(Cords c1, Cords c2) const {
    return (c1.x == c2.x && c1.y == c2.y);
}

bool eqstr::operator()(std::string s1, std::string s2) const {
    return (s1 == s2);
}

template <class T, class Sub>
bool lookup(T Set, Sub word) {
    typename T::const_iterator it = Set.find(word);
    return it != Set.end();
}


bool move_cmp(Move first, Move second) {
    return first.score > second.score;
}
