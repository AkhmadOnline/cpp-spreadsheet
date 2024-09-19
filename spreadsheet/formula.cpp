#include "formula.h"
#include "common.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

// Реализация оператора вывода для FormulaError
std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(expression))
        , expression_(std::move(expression)) 
    {
        // Удаляем лишние пробелы из выражения
        expression_.erase(std::remove_if(expression_.begin(), expression_.end(), isspace), expression_.end());
    } catch (const std::exception& exc) {
        throw FormulaException(exc.what());
    }

    FormulaInterface::Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        } catch (const FormulaError& fe) {
            return fe;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> cells(ast_.GetCells().begin(), ast_.GetCells().end());
        std::sort(cells.begin(), cells.end());

        // Удаляем дубликаты:
        cells.erase(std::unique(cells.begin(), cells.end()), cells.end());

        return cells;
    }

private:
    FormulaAST ast_;
    std::string expression_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}