// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "resultlist.h"
#include <ostream>

namespace document {
namespace select {

ResultList::ResultList() : _results() { }

ResultList::~ResultList() { }

ResultList::ResultList(const Result& result) {
    add(VariableMap(), result);
}

void
ResultList::add(const fieldvalue::VariableMap& variables, const Result& result)
{
    _results.push_back(ResultPair(variables, &result));
}

void
ResultList::print(std::ostream& out, bool, const std::string&) const
{
    out << "ResultList(";
    for (uint32_t i = 0; i < _results.size(); i++) {
        if (!_results[i].first.empty()) {
            out << _results[i].first.toString() << " => ";
        }
        out << _results[i].second->toString() << " ";
    }
    out << ")";
}

namespace {

const Result &
combineResultsLocal(const ResultList::Results & results)
{
    if (results.empty()) {
        return Result::False;
    }

    bool foundFalse = false;

    for (const auto & it : results) {
        if (*it.second == Result::True) {
            return Result::True;
        } else if (*it.second == Result::False) {
            foundFalse = true;
        }
    }

    return (foundFalse) ? Result::False : Result::Invalid;
}

}

const Result&
ResultList::combineResults() const {
    return combineResultsLocal(_results);
}

bool
ResultList::combineVariables(
        fieldvalue::VariableMap& output,
        const fieldvalue::VariableMap& input) const
{
    // First, verify that all variables are overlapping
    for (fieldvalue::VariableMap::const_iterator iter = output.begin(); iter != output.end(); iter++) {
        fieldvalue::VariableMap::const_iterator found(input.find(iter->first));

        if (found != input.end()) {
            if (!(found->second == iter->second)) {
                return false;
            }
        }
    }

    for (fieldvalue::VariableMap::const_iterator iter = input.begin(); iter != input.end(); iter++) {
        fieldvalue::VariableMap::const_iterator found(output.find(iter->first));
        if (found != output.end()) {
            if (!(found->second == iter->second)) {
                return false;
            }
        }
    }
    // Ok, variables are overlapping. Add all variables from input to output.
    for (fieldvalue::VariableMap::const_iterator iter = input.begin(); iter != input.end(); iter++) {
        output[iter->first] = iter->second;
    }

    return true;
}

ResultList
ResultList::operator&&(const ResultList& other) const
{
    ResultList result;

    // TODO: optimize
    for (const auto & it : _results) {
        for (const auto & it2 : other._results) {
            fieldvalue::VariableMap vars = it.first;

            if (combineVariables(vars, it2.first)) {
                result.add(vars, *it.second && *it2.second);
            }
        }
    }

    return result;
}

ResultList
ResultList::operator||(const ResultList& other) const
{
    ResultList result;

    // TODO: optimize
    for (const auto & it : _results) {
        for (const auto & it2 : other._results) {
            fieldvalue::VariableMap vars = it.first;

            if (combineVariables(vars, it2.first)) {
                result.add(vars, *it.second || *it2.second);
            }
        }
    }

    return result;
}

ResultList
ResultList::operator!() const {
    ResultList result;

    for (const auto & it : _results) {
        result.add(it.first, !*it.second);
    }

    return result;
}

bool ResultList::operator==(const ResultList& other) const {
    return (combineResultsLocal(_results) == combineResultsLocal(other._results));
}

} // select
} // document
