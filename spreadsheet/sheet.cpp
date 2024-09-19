#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    if (!cells_.count(pos)) {
        cells_[pos] = std::make_unique<Cell>(*this);
    }

    // Проверяем формулу на корректность:
    if (text.front() == FORMULA_SIGN && text.size() > 1) {
        try {
            ParseFormula(text.substr(1));
        } catch (const FormulaException&) {
            throw; // Перебрасываем исключение FormulaException
        }
    }

    Cell* cell = cells_[pos].get();
    cell->Set(text);

    if (HasCircularDependency(cell)) {
        cell->Clear();
        throw CircularDependencyException("Circular dependency detected");
    }

    UpdatePrintableSize(pos);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (cells_.count(pos)) {
        return cells_.at(pos).get();
    }
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (cells_.count(pos)) {
        return cells_.at(pos).get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (cells_.count(pos)) {
        cells_.erase(pos);

        // Пересчитываем printableSize_ после удаления ячейки
        printableSize_ = {0, 0};
        for (const auto& [position, cell] : cells_) {
            UpdatePrintableSize(position);
        }
    }
}

Size Sheet::GetPrintableSize() const {
    return printableSize_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int row = 0; row < printableSize_.rows; ++row) {
        for (int col = 0; col < printableSize_.cols; ++col) {
            Position pos{row, col};
            if (cells_.count(pos)) {
                std::visit(
                    [&](const auto& value) {
                        output << value;
                    },
                    cells_.at(pos)->GetValue());
            }
            if (col < printableSize_.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int row = 0; row < printableSize_.rows; ++row) {
        for (int col = 0; col < printableSize_.cols; ++col) {
            Position pos{row, col};
            if (cells_.count(pos)) {
                output << cells_.at(pos)->GetText();
            }
            if (col < printableSize_.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::UpdatePrintableSize(Position pos) {
    printableSize_.rows = std::max(printableSize_.rows, pos.row + 1);
    printableSize_.cols = std::max(printableSize_.cols, pos.col + 1);
}

bool Sheet::HasCircularDependency(Cell* startCell) {
    std::unordered_set<Cell*> visited;
    std::unordered_set<Cell*> path;

    std::function<bool(Cell*)> dfs = [&](Cell* cell) {
        if (visited.count(cell)) {
            return false; // Ячейка уже посещена
        }
        visited.insert(cell);
        path.insert(cell);

        for (Cell* nextCell : cell->GetDependentCells()) {
            if (path.count(nextCell)) {
                return true; 
            }
            if (dfs(nextCell)) {
                return true;
            }
        }

        path.erase(cell);
        return false;
    };

    return dfs(startCell);
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}