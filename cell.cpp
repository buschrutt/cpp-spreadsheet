#include "cell.h"

#include <iostream>
#include <string>

Cell::Cell(): impl_(std::make_unique<EmptyImpl>()){
    cell_node_.node_ptr = this;
}

Cell::~Cell() = default;

void Cell::Set(std::string text, SheetInterface &sheet) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else if (text.size() != 1 && text[0] == '=') {
        std::string old_val_text = cell_node_.node_ptr->GetText();
        impl_ = std::make_unique<FormulaImpl>(text.substr(1), sheet);
        //// Begin of --graph processing
        cell_node_.prev_ptr_set.clear();
        for (auto pos : this->GetReferencedCells()) {
            auto prev_node = dynamic_cast<Cell*>(sheet.GetCell(pos));
            prev_node->cell_node_.next_ptr_set.insert(cell_node_.node_ptr);
            cell_node_.prev_ptr_set.insert(prev_node);
        }
        try {
            ReferenceUpdate(cell_node_.node_ptr, cell_node_.prev_ptr_set, sheet);
        } catch (const CircularDependencyException&) {
            if (old_val_text.empty()) impl_ = std::make_unique<EmptyImpl>();
            else if (old_val_text.size() != 1 && old_val_text[0] == '=') impl_ = std::make_unique<FormulaImpl>(old_val_text.substr(1), sheet);
            else impl_ = std::make_unique<TextImpl>(old_val_text);
            throw CircularDependencyException("ReferenceUpdate --cycle found");
        }
        //// End Of --graph processing
    } else {
        impl_ = std::make_unique<TextImpl>(text);
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::ReferenceUpdate(Cell* node_ptr, std::set<Cell*> node_ptr_set, SheetInterface &sheet) {
    for (auto a : node_ptr_set){
        if (node_ptr == a) {
            throw CircularDependencyException("ReferenceUpdate --cycle found");
        }
    }
    node_ptr_set.emplace(node_ptr);
    for (auto a : node_ptr->cell_node_.next_ptr_set){
        ReferenceUpdate(a, node_ptr_set, sheet);
        a->CashUpdate(a->GetText(), sheet);
    }
}

void Cell::CashUpdate(const std::string& text, SheetInterface &sheet) {
    impl_ = std::make_unique<FormulaImpl>(text.substr(1), sheet);
}

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit([&](const auto& val) { output << val; }, value);
    return output;
}
