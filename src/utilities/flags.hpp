#pragma once

namespace Utilities {

template <typename EnumType, typename BaseType = int>
class Flags {
    public:
    Flags() : mask(0) {}
    Flags(EnumType value) : mask(static_cast<BaseType>(value)) {}
    Flags(const Flags<EnumType, BaseType> &other) : mask(other.mask) {}
    explicit Flags(BaseType mask) : mask(mask) {};

    void operator=(const Flags<EnumType, BaseType> &other) {
        mask = other.mask;
    }
    
    Flags<EnumType, BaseType> &operator|=(const Flags<EnumType, BaseType> &other) {
        mask |= other.mask;

        return *this;
    }
    Flags<EnumType, BaseType> &operator^=(const Flags<EnumType, BaseType> &other) {
        mask ^= other.mask;

        return *this;
    }
    Flags<EnumType, BaseType> &operator&=(const Flags<EnumType, BaseType> &other) {
        mask &= other.mask;

        return *this;
    }
    Flags<EnumType, BaseType> operator|(const Flags<EnumType, BaseType> &other) const {
        return Flags<EnumType, BaseType>{
            mask | other.mask
        };
    }
    Flags<EnumType, BaseType> operator^(const Flags<EnumType, BaseType> &other) const {
        return Flags<EnumType, BaseType>{
            mask ^ other.mask
        };
    }
    Flags<EnumType, BaseType> operator&(const Flags<EnumType, BaseType> &other) const {
        return Flags<EnumType, BaseType>{
            mask & other.mask
        };
    }

    Flags<EnumType, BaseType> operator~() const {
        return Flags<EnumType, BaseType>{
            ~mask
        };
    }

    bool operator==(const Flags<EnumType, BaseType> &other) const {
        return (mask == other.mask);
    }
    bool operator!=(const Flags<EnumType, BaseType> &other) const {
        return (mask != other.mask);
    }

    operator bool() const {
        return mask != 0;
    }

    private:
    BaseType mask;
};

template <typename EnumType>
Flags<EnumType> operator|(EnumType value, const Flags<EnumType> &flags) {
    return flags | value;
}

template <typename EnumType>
Flags<EnumType> operator&(EnumType value, const Flags<EnumType> &flags) {
    return flags & value;
}

template <typename EnumType>
Flags<EnumType> operator^(EnumType value, const Flags<EnumType> &flags) {
    return flags ^ value;
}

}