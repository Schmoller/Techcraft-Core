#pragma once

template<typename T>
class Badge {
    friend T;
public:
    Badge(const Badge &) = delete;
    Badge &operator=(const Badge &) = delete;

    Badge(Badge &&) = delete;
    Badge &operator=(Badge &&) = delete;

private:
    constexpr Badge() = default;
};
