#pragma once
#include "FormulaLexer.h"
#include "common.h"
#include <forward_list>
#include <functional>
#include <stdexcept>

namespace ASTImpl {
    class Expr;
}

class ParsingError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class FormulaAST {
public:
    explicit FormulaAST(std::unique_ptr<ASTImpl::Expr> root_expr, std::forward_list<Position>  cells);

    FormulaAST(FormulaAST&&) = default;

    FormulaAST& operator=(FormulaAST&&) = default;

    ~FormulaAST();

    [[nodiscard]] CellInterface::Value Execute(const SheetInterface& sheet) const; // Executes all Cells in the sheet

    void Print(std::ostream& out) const;

    void PrintFormula(std::ostream& out) const; // Prints formula

    [[maybe_unused]] std::forward_list<Position>& GetCells(); // Cells that affect the formula --cells_

    [[nodiscard]] const std::forward_list<Position>& GetCells() const; // Cells that affect the formula --cells_

private:
    std::unique_ptr<ASTImpl::Expr> root_expr_;
    std::forward_list<Position> cells_;
};

FormulaAST ParseFormulaAST(std::istream& in);

FormulaAST ParseFormulaAST(const std::string& in_str);