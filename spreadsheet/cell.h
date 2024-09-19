#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    const std::unordered_set<Cell*>& GetDependentCells() const;

    void AddReferencedCell(Cell* cell);
    void RemoveReferencedCell(Cell* cell);
    void AddDependentCell(Cell* cell);
    void RemoveDependentCell(Cell* cell);

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    Sheet& sheet_;
    std::unique_ptr<Impl> impl_;
    mutable std::optional<Value> cache_;
    mutable bool isCacheInvalidated_ = false;
    std::unordered_set<Cell*> referencedCells_;
    std::unordered_set<Cell*> dependentCells_;

    void InvalidateCache();
    void UpdateDependencies();
};

#include "sheet.h"