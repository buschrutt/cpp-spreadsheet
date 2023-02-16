#include "sheet.h"
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) throw InvalidPositionException("Sheet::SetCell");
    sheet_[pos].Set(text, *this);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) throw InvalidPositionException("Sheet::GetCell");
    if (sheet_.count(pos) > 0) return &sheet_.at(pos);
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) throw InvalidPositionException("Sheet::GetCell");
    if (sheet_.count(pos) > 0) return &sheet_.at(pos);
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) throw InvalidPositionException("Sheet::ClearCell");
    auto it = sheet_.find(pos);
    if (it != sheet_.end()) sheet_.erase(it);
}

Size Sheet::GetPrintableSize() const {
    if (sheet_.begin() == sheet_.end()) return { 0, 0 };
    Size max_size = {0, 0};
    auto runner = sheet_.begin();
    while (runner != sheet_.end()) {
        if (runner->first.row > max_size.rows) max_size.rows = runner->first.row;
        if (runner->first.col > max_size.cols) max_size.cols = runner->first.col;
        runner++;
    }
    return {max_size.rows + 1, max_size.cols + 1};
}

void Sheet::PrintValues(std::ostream& output) const {
    bool flag = true;
    Size printable_size = GetPrintableSize();
    for (int i = 0; i < printable_size.rows; i++) {
        for (int j = 0; j < printable_size.cols; j++) {
            !flag ? output << '\t' : output;
            flag = false;
            if (sheet_.count({i, j}) > 0) {
                Cell::Value value = sheet_.at({i, j}).GetValue();
                if (std::holds_alternative<double>(value)) output << std::get<double>(value);
                else if (std::holds_alternative<std::string>(value)) output << std::get<std::string>(value);
                else if (std::holds_alternative<FormulaError>(value)) output << std::get<FormulaError>(value);
                else throw FormulaException ("Sheet::PrintValues: --Unknown exception");
            }
        }
        output << '\n';
        flag = true;
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    bool flag = true;
    Size printable_size = GetPrintableSize();
    for (int i = 0; i < printable_size.rows; i++) {
        for (int j = 0; j < printable_size.cols; j++) {
            !flag ? output << '\t' : output;
            flag = false;
            if (sheet_.count({i, j}) > 0) output << sheet_.at({i, j}).GetText();
        }
        output << '\n';
        flag = true;
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}