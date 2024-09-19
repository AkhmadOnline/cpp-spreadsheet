#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <map>
#include <memory>

class Cell;

class Sheet : public SheetInterface {
public:
    ~Sheet() override;

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::map<Position, std::unique_ptr<Cell>> cells_;
    Size printableSize_ = {0, 0};

    void UpdatePrintableSize(Position pos);
    bool HasCircularDependency(Cell* startCell);
};