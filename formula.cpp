#include "formula.h"
#include "FormulaAST.h"

#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string& expression): ast_(ParseFormulaAST(expression)) {}

        [[nodiscard]] Value Evaluate(const SheetInterface& sheet) const override {
            try {
                if (std::holds_alternative<double>(ast_.Execute(sheet))) {
                    return std::get<double>(ast_.Execute(sheet));
                }
                return std::get<FormulaError>(ast_.Execute(sheet));
            } catch (FormulaError & fe) {
                return fe;
            }
        }

        [[nodiscard]] std::string GetExpression() const override {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

        [[nodiscard]] std::vector<Position> GetReferencedCells() const override {
            return {ast_.GetCells().begin(), ast_.GetCells().end()};
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

[[maybe_unused]] [[maybe_unused]] std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(expression);
    } catch (...) {
        throw FormulaException("flag-- FormulaException");
    }
}