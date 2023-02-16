#pragma once

#include <utility>

#include "common.h"
#include "formula.h"
#include "sheet.h"

class Cell;

struct CellNode {
public:
    Cell* node_ptr = nullptr;
    std::set<Cell*> next_ptr_set;
    std::set<Cell*> prev_ptr_set;
};

class Impl {
public:
    virtual std::string GetText() = 0;

    virtual CellInterface::Value GetValue() = 0;

    virtual std::vector<Position> GetReferencedCells() = 0;
};

// Cell is empty, if value is requested - returns 0.0
class EmptyImpl : public Impl {
public:
    EmptyImpl()= default;

    [[nodiscard]] std::string GetText() override {return "";}

    [[nodiscard]] CellInterface::Value GetValue() override {return 0.0;}

    std::vector<Position> GetReferencedCells() override {return {};}

};

// Cell as a text
class TextImpl : public Impl {
public:
    explicit TextImpl(std::string text): text_(std::move(text)) {}

    [[nodiscard]] std::string GetText() override {return text_;}

    [[nodiscard]] CellInterface::Value GetValue() override {
        if (text_[0] == '\''){
            return text_.substr(1);
        }
        return text_;
    }
    std::vector<Position> GetReferencedCells() override {return {};}

private:
    std::string text_;
};

// Cell as a formula
class FormulaImpl : public Impl {
public:
    explicit FormulaImpl(const std::string &expression, SheetInterface &sheet) : sheet_(sheet) {
        formula_ = ParseFormula(expression);
        if (std::holds_alternative<double>(formula_->Evaluate(sheet_))) {
            cash_ = std::get<double>(formula_->Evaluate(sheet_));
            referenced_cells_ = formula_->GetReferencedCells();
            for (auto pos : referenced_cells_) {
                if (!sheet.GetCell(pos)) sheet.SetCell(pos, "");
            }
        } else if (std::holds_alternative<FormulaError>(formula_->Evaluate(sheet_))) {
            cash_ = std::get<FormulaError>(formula_->Evaluate(sheet_));
        }
    }

    [[nodiscard]] std::string GetText() override {return "=" + formula_->GetExpression();}

    [[nodiscard]] CellInterface::Value GetValue() override {
        return cash_; // We keep cash that was calculated within last call --constructor. If cash is invalid - the new valid data rewrites with new constructor call
    }

    std::vector<Position> GetReferencedCells() override {return referenced_cells_;}

private:
    std::unique_ptr<FormulaInterface> formula_;
    SheetInterface& sheet_;
    CellInterface::Value cash_ = 0;
    std::vector<Position> referenced_cells_{};
};

class Cell : public CellInterface {
public:
    Cell();

    ~Cell() override;

    void Set(std::string text, SheetInterface &sheet); // Sets new cell

    void Clear();

    [[nodiscard]] Value GetValue() const override; // Gets cell value

    [[nodiscard]] std::string GetText() const override; // Gets cell value as a string

    [[nodiscard]] std::vector<Position> GetReferencedCells() const override; // Gets all cells that are used in formula

    void ReferenceUpdate(Cell* node_ptr, std::set<Cell*> node_ptr_set, SheetInterface &sheet); // Updates graph and calls CashUpdate for invalid cells --recursive

    void CashUpdate(const std::string& text, SheetInterface &sheet); // Updates cell's cash after invalidation

private:
    std::unique_ptr<Impl> impl_; // Cell data
    CellNode cell_node_; // Structure for dependencies graph implementation
};

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value);