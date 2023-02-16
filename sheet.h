#pragma once
#include "cell.h"
#include "common.h"
#include <functional>
#include <unordered_map>
#include <map>

class Sheet : public SheetInterface {
public:

    friend class Cell; //access to Sheet methods from cell

    using Sheet_data = std::map<Position, Cell>;

    ~Sheet() override;

    void SetCell(Position pos, std::string text) override; // Creating and setting Cell in sheet_ by key pos

    [[nodiscard]] const CellInterface* GetCell(Position pos) const override; // Access to CellInterface ptr

    CellInterface* GetCell(Position pos) override; // Access to CellInterface ptr

    void ClearCell(Position pos) override; // Setting impl_ to nullptr

    [[nodiscard]] Size GetPrintableSize() const override; // Printable area of the sheet

    void PrintValues(std::ostream& output) const override; // Printing sheet existing values in printable area

    void PrintTexts(std::ostream& output) const override; // Printing sheet existing values as text in printable area

private:
    Sheet_data sheet_{}; // Structure for keeping sheet data
};

