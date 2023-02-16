#pragma once
#include "common.h"
#include <memory>
#include <variant>

// Формула, позволяющая вычислять и обновлять арифметическое выражение.
// Поддерживаемые возможности:
// * Простые бинарные операции и числа, скобки: 1+2*3, 2.5*(2+3.5/7)
class FormulaInterface {
public:
    using Value = std::variant<double, FormulaError>;

    virtual ~FormulaInterface() = default;

    // Возвращает вычисленное значение формулы либо ошибку. На данном этапе
    // мы создали только 1 вид ошибки -- деление на 0.
    [[maybe_unused]] [[nodiscard]] virtual Value Evaluate(const SheetInterface& sheet) const = 0;

    // Возвращает выражение, которое описывает формулу.
    // Не содержит пробелов и лишних скобок.
    [[maybe_unused]] [[nodiscard]] virtual std::string GetExpression() const = 0;


    // Возвращает список ячеек, которые непосредственно задействованы в вычислении
    // формулы. Список отсортирован по возрастанию и не содержит повторяющихся ячеек.
    [[nodiscard]] virtual std::vector<Position> GetReferencedCells() const = 0;
};

// Парсит переданное выражение и возвращает объект формулы.
// Бросает FormulaException в случае, если формула синтаксически некорректна.
[[maybe_unused]] std::unique_ptr<FormulaInterface> ParseFormula(std::string expression);