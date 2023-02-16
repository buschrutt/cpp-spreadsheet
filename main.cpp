#include <utility>

#include "cell.h" // ???
#include "common.h"
#include "formula.h"
#include "test_runner_p.h"

inline std::ostream& operator<<(std::ostream& output, Position pos) {
    return output << "(" << pos.row << ", " << pos.col << ")";
}

inline Position operator"" _pos(const char* str, std::size_t) {
    return Position::FromString(str);
}

inline std::ostream& operator<<(std::ostream& output, Size size) {
    return output << "(" << size.rows << ", " << size.cols << ")";
}

//inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    //std::visit([&](const auto& x) {output << x;}, value); return output; }

namespace {
    [[maybe_unused]] std::string ToString(FormulaError::Category category) {
        return std::string(FormulaError(category).ToString());
    }

    void TestPositionAndStringConversion() {
        auto testSingle = [](Position pos, std::string_view str) {
            ASSERT_EQUAL(pos.ToString(), str)
            ASSERT_EQUAL(Position::FromString(str), pos)
        };

        for (int i = 0; i < 25; ++i) {
            testSingle(Position{i, i}, char('A' + i) + std::to_string(i + 1));
        }

        testSingle(Position{0, 0}, "A1");
        testSingle(Position{0, 1}, "B1");
        testSingle(Position{0, 25}, "Z1");
        testSingle(Position{0, 26}, "AA1");
        testSingle(Position{0, 27}, "AB1");
        testSingle(Position{0, 51}, "AZ1");
        testSingle(Position{0, 52}, "BA1");
        testSingle(Position{0, 53}, "BB1");
        testSingle(Position{0, 77}, "BZ1");
        testSingle(Position{0, 78}, "CA1");
        testSingle(Position{0, 701}, "ZZ1");
        testSingle(Position{0, 702}, "AAA1");
        testSingle(Position{136, 2}, "C137");
        testSingle(Position{Position::MAX_ROWS - 1, Position::MAX_COLS - 1}, "XFD16384");
    }

    void TestPositionToStringInvalid() {
        ASSERT_EQUAL((Position{-1, -1}).ToString(), "")
        ASSERT_EQUAL((Position{-10, 0}).ToString(), "")
        ASSERT_EQUAL((Position{1, -3}).ToString(), "")
    }

    void TestStringToPositionInvalid() {
        ASSERT(!Position::FromString("").IsValid())
        ASSERT(!Position::FromString("A").IsValid())
        ASSERT(!Position::FromString("1").IsValid())
        ASSERT(!Position::FromString("e2").IsValid())
        ASSERT(!Position::FromString("A0").IsValid())
        ASSERT(!Position::FromString("A-1").IsValid())
        ASSERT(!Position::FromString("A+1").IsValid())
        ASSERT(!Position::FromString("R2D2").IsValid())
        ASSERT(!Position::FromString("C3PO").IsValid())
        ASSERT(!Position::FromString("XFD16385").IsValid())
        ASSERT(!Position::FromString("XFE16384").IsValid())
        ASSERT(!Position::FromString("A1234567890123456789").IsValid())
        ASSERT(!Position::FromString("ABCDEFGHIJKLMNOPQRS8").IsValid())
    }

    void TestEmpty() {
        auto sheet = CreateSheet();
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{0, 0}))
    }

    void TestInvalidPosition() {
        auto sheet = CreateSheet();
        try {
            sheet->SetCell(Position{-1, 0}, "");
        } catch (const InvalidPositionException&) {
        }
        try {
            sheet->GetCell(Position{0, -2});
        } catch (const InvalidPositionException&) {
        }
        try {
            sheet->ClearCell(Position{Position::MAX_ROWS, 0});
        } catch (const InvalidPositionException&) {
        }
    }

    void TestSetCellPlainText() {
        auto sheet = CreateSheet();

        auto checkCell = [&](Position pos, const std::string& text) {
            sheet->SetCell(pos, text);
            CellInterface* cell = sheet->GetCell(pos);
            ASSERT(cell != nullptr)
            ASSERT_EQUAL(cell->GetText(), text)
            ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), text)
        };

        checkCell("A1"_pos, "Hello");
        checkCell("A1"_pos, "World");
        checkCell("B2"_pos, "Purr");
        checkCell("A3"_pos, "Meow");

        const SheetInterface& constSheet = *sheet;
        ASSERT_EQUAL(constSheet.GetCell("B2"_pos)->GetText(), "Purr")

        sheet->SetCell("A3"_pos, "'=escaped");
        CellInterface* cell = sheet->GetCell("A3"_pos);
        ASSERT_EQUAL(cell->GetText(), "'=escaped")
        ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), "=escaped")
    }

    void TestClearCell() {
        auto sheet = CreateSheet();

        sheet->SetCell("C2"_pos, "Me gusta");
        sheet->ClearCell("C2"_pos);
        ASSERT(sheet->GetCell("C2"_pos) == nullptr)

        sheet->ClearCell("A1"_pos);
        sheet->ClearCell("J10"_pos);
    }

    void TestFormulaArithmetic() {
        auto sheet = CreateSheet();
        auto evaluate = [&](std::string expr) {
            return std::get<double>(ParseFormula(std::move(expr))->Evaluate(*sheet));
        };

        ASSERT_EQUAL(evaluate("1"), 1)
        ASSERT_EQUAL(evaluate("42"), 42)
        ASSERT_EQUAL(evaluate("2 + 2"), 4)
        ASSERT_EQUAL(evaluate("2 + 2*2"), 6)
        ASSERT_EQUAL(evaluate("4/2 + 6/3"), 4)
        ASSERT_EQUAL(evaluate("(2+3)*4 + (3-4)*5"), 15)
        ASSERT_EQUAL(evaluate("(12+13) * (14+(13-24/(1+1))*55-46)"), 575)
    }

    void TestFormulaReferences() {
        auto sheet = CreateSheet();
        auto evaluate = [&](std::string expr) {
            return std::get<double>(ParseFormula(std::move(expr))->Evaluate(*sheet));
        };

        sheet->SetCell("A1"_pos, "1");
        ASSERT_EQUAL(evaluate("A1"), 1)
        sheet->SetCell("A2"_pos, "2");
        ASSERT_EQUAL(evaluate("A1+A2"), 3)

        // Division by zero test:
        sheet->SetCell("B3"_pos, "");
        ASSERT_EQUAL(evaluate("A1+B3"), 1)  // Empty text cell
        ASSERT_EQUAL(evaluate("A1+B1"), 1)  // Empty cell
        ASSERT_EQUAL(evaluate("A1+E4"), 1)  // Cell is out of the sheet
    }

    void TestFormulaExpressionFormatting() {
        auto reformat = [](std::string expr) {
            return ParseFormula(std::move(expr))->GetExpression();
        };

        ASSERT_EQUAL(reformat("  1  "), "1")
        ASSERT_EQUAL(reformat("  -1  "), "-1")
        ASSERT_EQUAL(reformat("2 + 2"), "2+2")
        ASSERT_EQUAL(reformat("(2*3)+4"), "2*3+4")
        ASSERT_EQUAL(reformat("(2*3)-4"), "2*3-4")
        ASSERT_EQUAL(reformat("( ( (  1) ) )"), "1")
    }

    void TestFormulaReferencedCells() {
        ASSERT(ParseFormula("1")->GetReferencedCells().empty())

        auto a1 = ParseFormula("A1");
        ASSERT_EQUAL(a1->GetReferencedCells(), (std::vector{"A1"_pos}))

        auto b2c3 = ParseFormula("B2+C3");
        ASSERT_EQUAL(b2c3->GetReferencedCells(), (std::vector{"B2"_pos, "C3"_pos}))

        auto tricky = ParseFormula("A1 + A2 + A1 + A3 + A1 + A2 + A1");
        ASSERT_EQUAL(tricky->GetExpression(), "A1+A2+A1+A3+A1+A2+A1")
        ASSERT_EQUAL(tricky->GetReferencedCells(), (std::vector{"A1"_pos, "A2"_pos, "A3"_pos}))
    }

    void TestErrorValue() {
        auto sheet = CreateSheet();
        sheet->SetCell("E2"_pos, "A1");
        sheet->SetCell("E4"_pos, "=E2");
        ASSERT_EQUAL(sheet->GetCell("E4"_pos)->GetValue(), CellInterface::Value(FormulaError::Category::Value))

        sheet->SetCell("E2"_pos, "3D");
        ASSERT_EQUAL(sheet->GetCell("E4"_pos)->GetValue(), CellInterface::Value(FormulaError::Category::Value))
    }

    void TestErrorDiv0() {
        auto sheet = CreateSheet();

        constexpr double max = std::numeric_limits<double>::max();

        sheet->SetCell("A1"_pos, "=1/0");
        ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), CellInterface::Value(FormulaError::Category::Div0))

        sheet->SetCell("A1"_pos, "=1e+200/1e-200");
        ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), CellInterface::Value(FormulaError::Category::Div0))

        sheet->SetCell("A1"_pos, "=0/0");
        ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), CellInterface::Value(FormulaError::Category::Div0))

        {
            std::ostringstream formula;
            formula << '=' << max << '+' << max;
            sheet->SetCell("A1"_pos, formula.str());
            ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), CellInterface::Value(FormulaError::Category::Div0))
        }

        {
            std::ostringstream formula;
            formula << '=' << -max << '-' << max;
            sheet->SetCell("A1"_pos, formula.str());
            ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), CellInterface::Value(FormulaError::Category::Div0))
        }

        {
            std::ostringstream formula;
            formula << '=' << max << '*' << max;
            sheet->SetCell("A1"_pos, formula.str());
            ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), CellInterface::Value(FormulaError::Category::Div0))
        }
    }

    void TestEmptyCellTreatedAsZero() {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "=B2");
        ASSERT_EQUAL(sheet->GetCell("A1"_pos)->GetValue(), CellInterface::Value(0))
    }

    void TestFormulaInvalidPosition() {
        auto sheet = CreateSheet();
        auto try_formula = [&](const std::string& formula) {
            try {
                sheet->SetCell("A1"_pos, formula);
                ASSERT(false)
            } catch (const FormulaException&) {
                // we expect this one
            }
        };

        try_formula("=X0");
        try_formula("=ABCD1");
        try_formula("=A123456");
        try_formula("=ABCDEFGHIJKLMNOPQRS1234567890");
        try_formula("=XFD16385");
        try_formula("=XFE16384");
        try_formula("=R2D2");
    }

    void TestPrint() {
        auto sheet = CreateSheet();
        sheet->SetCell("A2"_pos, "meow");
        sheet->SetCell("B2"_pos, "=35");

        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{2, 2}))

        std::ostringstream texts;
        sheet->PrintTexts(texts);
        ASSERT_EQUAL(texts.str(), "\t\nmeow\t=35\n")

        std::ostringstream values;
        sheet->PrintValues(values);
        ASSERT_EQUAL(values.str(), "\t\nmeow\t35\n")
    }

    void TestCellReferences() {
        auto sheet = CreateSheet();
        sheet->SetCell("A1"_pos, "1");
        sheet->SetCell("A2"_pos, "=A1");
        sheet->SetCell("B2"_pos, "=A1");

        ASSERT(sheet->GetCell("A1"_pos)->GetReferencedCells().empty())
        ASSERT_EQUAL(sheet->GetCell("A2"_pos)->GetReferencedCells(), std::vector{"A1"_pos})
        ASSERT_EQUAL(sheet->GetCell("B2"_pos)->GetReferencedCells(), std::vector{"A1"_pos})

        // Ссылка на пустую ячейку
        sheet->SetCell("B2"_pos, "=B1");
        ASSERT(sheet->GetCell("B1"_pos)->GetReferencedCells().empty())
        ASSERT_EQUAL(sheet->GetCell("B2"_pos)->GetReferencedCells(), std::vector{"B1"_pos})

        sheet->SetCell("A2"_pos, "");
        ASSERT(sheet->GetCell("A1"_pos)->GetReferencedCells().empty())
        ASSERT(sheet->GetCell("A2"_pos)->GetReferencedCells().empty())

        // Ссылка на ячейку за пределами таблицы
        sheet->SetCell("B1"_pos, "=C3");
        ASSERT_EQUAL(sheet->GetCell("B1"_pos)->GetReferencedCells(), std::vector{"C3"_pos})
    }

    void TestFormulaIncorrect() {
        auto isIncorrect = [](std::string expression) {
            try {
                ParseFormula(std::move(expression));
            } catch (const FormulaException&) {
                return true;
            }
            return false;
        };

        ASSERT(isIncorrect("A2B"))
        ASSERT(isIncorrect("3X"))
        ASSERT(isIncorrect("A0++"))
        ASSERT(isIncorrect("((1)"))
        ASSERT(isIncorrect("2+4-"))
    }

    void TestCellCircularReferences() {
        auto sheet = CreateSheet();
        sheet->SetCell("E2"_pos, "=E4");
        sheet->SetCell("E4"_pos, "=X9");
        sheet->SetCell("X9"_pos, "=M6");
        sheet->SetCell("M6"_pos, "Ready");

        bool caught = false;
        try {
            sheet->SetCell("M6"_pos, "=E2");
        } catch (const CircularDependencyException&) {
            caught = true;
        }

        ASSERT(caught)
        ASSERT_EQUAL(sheet->GetCell("M6"_pos)->GetText(), "Ready")
    }

    void TestSetGetCellFormulaValid() {
        auto sheet = CreateSheet();
        auto checkCell = [&sheet](Position pos, std::string text) {
            sheet->SetCell(pos, std::move(text));
            {
                CellInterface* cell = sheet->GetCell(pos);
                assert(cell != nullptr);
                std::cout << cell->GetText() << std::endl;
                std::cout << std::get<double>(cell->GetValue()) << std::endl;
            } {
            const auto& sheet_c = sheet;
            CellInterface* cell = sheet_c->GetCell(pos);
            assert(cell != nullptr);
            std::cout << cell->GetText() << std::endl;
            std::cout << std::get<double>(cell->GetValue()) << std::endl;
        }
        };
        checkCell("A1"_pos, "=1");
        checkCell("A1"_pos, "=1+2");
        checkCell("B2"_pos, "=1/2");
        checkCell("A3"_pos, "=(1+1)/-1");
        checkCell("C3"_pos, "=(1+1)/(+1)");
    }

    void TestSetGetCellCellRef() {
        auto sheet = CreateSheet();
        auto checkCell = [&sheet](Position pos, std::string text) {
            sheet->SetCell(pos, std::move(text));
            {
                CellInterface* cell = sheet->GetCell(pos);
                assert(cell != nullptr);
                std::cout << cell->GetText() << std::endl;
                std::cout << std::get<double>(cell->GetValue()) << std::endl;
            } {
            const auto& sheet_c = sheet;
            CellInterface* cell = sheet_c->GetCell(pos);
            assert(cell != nullptr);
            std::cout << cell->GetText() << std::endl;
            std::cout << std::get<double>(cell->GetValue()) << std::endl;
        }
        };

        checkCell("A1"_pos, "=1");
        checkCell("B2"_pos, "=1/2");
        checkCell("A3"_pos, "=(1+1)/-1");
        checkCell("C3"_pos, "=(1+1)/(+1)");

        checkCell("A2"_pos, "=A1");
        checkCell("B3"_pos, "=B2+(12/3 - 2)");
        checkCell("A4"_pos, "=A3+C3");
        checkCell("C4"_pos, "=C3 + B2 / C3");
        checkCell("D1"_pos, "=A1 + A1");
    }

    void TestSetGetCellFormulaZeroDivision() {
        auto sheet = CreateSheet();
        auto checkCell = [&sheet](Position pos, std::string text) {
            sheet->SetCell(pos, std::move(text)); /// --!-- check here
            {
                CellInterface* cell = sheet->GetCell(pos);
                assert(cell != nullptr);
                std::cout << cell->GetText() << std::endl;
                std::cout << std::get<FormulaError>(cell->GetValue()) << std::endl;
            } {
            const auto& sheet_c = sheet;
            CellInterface* cell = sheet_c->GetCell(pos);
            assert(cell != nullptr);
            std::cout << cell->GetText() << std::endl;
            std::cout << std::get<FormulaError>(cell->GetValue()) << std::endl;
        }
        };

        checkCell("A1"_pos, "=1/0");
        checkCell("A1"_pos, "=0/0");

        checkCell("B1"_pos, "=1/(1-1)");
        checkCell("B1"_pos, "=0/(1-1)");
        checkCell("B1"_pos, "=(1-1)/(1-1)");

        checkCell("C1"_pos, "=1+1/(1-1)");
        checkCell("C1"_pos, "=1+0/(1-1)");
        checkCell("C1"_pos, "=1+(1-1)/(1-1)");

        checkCell("D1"_pos, "=1/(1-1)+1");
        checkCell("D1"_pos, "=0/(1-1)+1");
        checkCell("D1"_pos, "=(1-1)/(1-1)+1");
    }

    void TestPachkaHelps() {
        auto sheet = CreateSheet();
        Position pos = "A1"_pos;
        sheet->SetCell(pos, "=1/1");
        CellInterface* cell = sheet->GetCell(pos);
        std::cout << cell->GetText() << std::endl;
        std::cout << std::get<double>(cell->GetValue()) << std::endl;
    }

}  // namespace


int main() {
    TestRunner tr;
    /* */
    RUN_TEST(tr, TestPositionAndStringConversion); /// --Ok
    RUN_TEST(tr, TestPositionToStringInvalid); /// --Ok
    RUN_TEST(tr, TestStringToPositionInvalid); /// --Ok
    RUN_TEST(tr, TestEmpty); /// --Ok
    RUN_TEST(tr, TestInvalidPosition); /// --Ok
    RUN_TEST(tr, TestSetCellPlainText); /// --Ok
    RUN_TEST(tr, TestClearCell); /// --Ok
    RUN_TEST(tr, TestFormulaArithmetic); /// --Ok
    RUN_TEST(tr, TestFormulaReferences); /// --Ok
    RUN_TEST(tr, TestFormulaExpressionFormatting); /// --Ok
    RUN_TEST(tr, TestFormulaReferencedCells); /// --Ok
    RUN_TEST(tr, TestErrorValue); /// --Ok
    RUN_TEST(tr, TestErrorDiv0); /// --Ok
    RUN_TEST(tr, TestEmptyCellTreatedAsZero); /// --Ok
    RUN_TEST(tr, TestFormulaInvalidPosition); /// --Ok
    RUN_TEST(tr, TestPrint); /// --Ok
    RUN_TEST(tr, TestCellReferences); /// --Ok
    RUN_TEST(tr, TestFormulaIncorrect); /// --Ok
    RUN_TEST(tr, TestCellCircularReferences); /// --Ok
    RUN_TEST(tr, TestSetGetCellFormulaValid); /// --main exits with zero
    RUN_TEST(tr, TestSetGetCellCellRef); /// --main exits with zero
    RUN_TEST(tr, TestSetGetCellFormulaZeroDivision); /// --main exits with zero
    RUN_TEST(tr, TestPachkaHelps); /// --main exits with zero
    return 0;
}