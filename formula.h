#pragma once
#include "common.h"
#include <memory>
#include <variant>

//// A formula that evaluates and updates an arithmetic expression.
//// Supported: Simple binary operations and numbers, brackets: 1+2*3, 2.5*(2+3.5/7)
class FormulaInterface {
public:
    using Value = std::variant<double, FormulaError>;

    virtual ~FormulaInterface() = default;

    // Returns the calculated value or an error
    [[maybe_unused]] [[nodiscard]] virtual Value Evaluate(const SheetInterface& sheet) const = 0;

    // Returns an expression. Does not contain spaces or extra parentheses
    [[maybe_unused]] [[nodiscard]] virtual std::string GetExpression() const = 0;


    // Returns a list of cells that are used for in the formula calculation with no duplicate cells
    [[nodiscard]] virtual std::vector<Position> GetReferencedCells() const = 0;
};

// Parses the expression and returns the formula object. Throws FormulaException if the formula is syntactically incorrect.
[[maybe_unused]] std::unique_ptr<FormulaInterface> ParseFormula(std::string expression);