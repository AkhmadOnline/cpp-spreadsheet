#include "cell.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <string>

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    CellInterface::Value GetValue() const override {
        return "";
    }
    std::string GetText() const override {
        return "";
    }
};

class Cell::TextImpl : public Cell::Impl {
public:
    explicit TextImpl(std::string text) : text_(std::move(text)) {}

    CellInterface::Value GetValue() const override {
        if (!text_.empty() && text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }

    std::string GetText() const override {
        return text_;
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    explicit FormulaImpl(const std::string& expression)
        : formula_(ParseFormula(expression)) {}

    CellInterface::Value GetValue() const override {
        return "";
    }

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    CellInterface::Value Evaluate(const SheetInterface& sheet) const {
        try {
            auto result = formula_->Evaluate(sheet);
            if (std::holds_alternative<double>(result)) {
                return std::get<double>(result);
            } else {
                return std::get<FormulaError>(result);
            }
        } catch (const FormulaError& error) {
            return error;
        }
    }

    std::vector<Position> GetReferencedCells() const {
        return formula_->GetReferencedCells();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
};

Cell::Cell(Sheet& sheet) : sheet_(sheet) {}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    cache_.reset();
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else if (text.front() == FORMULA_SIGN) {
        if (text.size() > 1) {
            try {
                impl_ = std::make_unique<FormulaImpl>(text.substr(1));
            } catch (const FormulaException& e) {
                impl_ = std::make_unique<TextImpl>(text);
            }
        } else {
            impl_ = std::make_unique<TextImpl>(text);
        }
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
    InvalidateCache();
    UpdateDependencies();
}

void Cell::Clear() {
    Set("");
}

CellInterface::Value Cell::GetValue() const {
    if (cache_) {
        return *cache_;
    }
    if (!impl_) {
        return "";
    }
    if (auto* formulaImpl = dynamic_cast<FormulaImpl*>(impl_.get())) {
        cache_ = formulaImpl->Evaluate(static_cast<const SheetInterface&>(sheet_)); 
    } else {
        cache_ = impl_->GetValue();
    }
    return *cache_;
}

std::string Cell::GetText() const {
    return impl_ ? impl_->GetText() : "";
}

std::vector<Position> Cell::GetReferencedCells() const {
    if (auto* formulaImpl = dynamic_cast<FormulaImpl*>(impl_.get())) {
        return formulaImpl->GetReferencedCells();
    }
    return {};
}

const std::unordered_set<Cell*>& Cell::GetDependentCells() const {
    return referencedCells_;
}

void Cell::AddReferencedCell(Cell* cell) {
    referencedCells_.insert(cell);
}

void Cell::RemoveReferencedCell(Cell* cell) {
    referencedCells_.erase(cell);
}

void Cell::AddDependentCell(Cell* cell) {
    dependentCells_.insert(cell);
}

void Cell::RemoveDependentCell(Cell* cell) {
    dependentCells_.erase(cell);
}

void Cell::InvalidateCache() {
    if (isCacheInvalidated_) {
        return;
    }
    cache_.reset();
    isCacheInvalidated_ = true;
    for (Cell* cell : dependentCells_) {
        cell->InvalidateCache();
    }
}

void Cell::UpdateDependencies() {
    for (Cell* oldCell : referencedCells_) {
        oldCell->RemoveDependentCell(this);
    }
    referencedCells_.clear();

    if (auto* formulaImpl = dynamic_cast<FormulaImpl*>(impl_.get())) {
        for (const Position& pos : formulaImpl->GetReferencedCells()) {
            if (sheet_.GetCell(pos)) {
                Cell* cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
                referencedCells_.insert(cell);
                cell->AddDependentCell(this);
            }
        }
    }
}