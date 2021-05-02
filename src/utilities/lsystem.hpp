#pragma once

#include <vector>
#include <random>
#include <string>
#include <assert.h>
#include <memory>

namespace Utilities::LSystem {

class Rule {
    public:
    const char match;

    Rule(char match, const std::string &transformation)
        : match(match), transformation(transformation)
    {}
    Rule(const Rule &other) = default;
    
    virtual int priority() const = 0;
    virtual bool requireRandon() const { return false; }

    virtual bool doesApply(char current, const std::string &string, size_t index, std::mt19937 *rand) = 0;

    size_t apply(std::string &vector, size_t index);
    bool operator<(const Rule &other) const;

    virtual std::string toString() const = 0;

    protected:
    std::string transformation;
};

/**
 * A basic context free deterministic rule.
 * If the input matches, then the rule will apply.
 */
class BasicRule: public Rule {
    public:
    BasicRule(char match, const std::string &transformation)
        : Rule(match, transformation)
    {}

    int priority() const override { return 0; }

    bool doesApply(char current, const std::string &string, size_t index, std::mt19937 *rand) override;
    std::string toString() const override;

};

/**
 * This rule expands on the basic rule allowing for probabilistic application.
 * The sum of all probabilities do not need to add up to 1. If none are chosen,
 * the identity transformation is applied.
 */
class ProbabilisticRule: public Rule {
    public:
    const float probability;

    ProbabilisticRule(char match, float probability, const std::string &transformation)
        : Rule(match, transformation), probability(probability)
    {}

    int priority() const override { return 1; }
    bool requireRandon() const override { return true; }

    bool doesApply(char current, const std::string &string, size_t index, std::mt19937 *rand) override;
    std::string toString() const override;
};

/**
 * This rule allows for contextual operations.
 */
class ContextualRule: public Rule {
    public:
    ContextualRule(char match, const std::string &before, const std::string &after, const std::string &transformation)
        : Rule(match, transformation), before(before), after(after)
    {}

    int priority() const override { return 2; }

    bool doesApply(char current, const std::string &string, size_t index, std::mt19937 *rand) override;
    std::string toString() const override;

    private:
    const std::string before;
    const std::string after;
};

class LSystem {
    public:
    LSystem();
    LSystem(const std::vector<std::shared_ptr<Rule>> &rules);

    void addRule(const std::shared_ptr<Rule> &rule);
    void addRule(char match, const std::string &transform);
    void addRule(char match, float probability, const std::string &transform);
    void addRule(char match, const std::string &before, const std::string &after, const std::string &transform);

    void setRand(std::mt19937 &rand);

    std::string run(char axiom, uint32_t iterations);
    std::string run(const std::string &axiom, uint32_t iterations);

    std::string toString() const;

    private:
    std::vector<std::shared_ptr<Rule>> rules;

    std::mt19937 *rand { nullptr };

    bool requireRand { false };
};

// Debug methods

std::string toString(const Rule &rule);
std::string toString(const BasicRule &rule);
std::string toString(const ProbabilisticRule &rule);
std::string toString(const ContextualRule &rule);


}