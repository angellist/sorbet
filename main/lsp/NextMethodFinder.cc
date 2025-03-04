#include "NextMethodFinder.h"
#include "core/GlobalState.h"

using namespace std;

namespace sorbet::realmain::lsp {

ast::ExpressionPtr NextMethodFinder::preTransformClassDef(core::Context ctx, ast::ExpressionPtr tree) {
    auto &classDef = ast::cast_tree_nonnull<ast::ClassDef>(tree);

    auto loc = core::Loc(ctx.file, tree.loc());

    if (!this->narrowestClassDefRange.exists()) {
        // No narrowestClassDefRange yet, so take the <root> loc
        ENFORCE(classDef.symbol == core::Symbols::root());
        this->narrowestClassDefRange = loc;
    } else if (loc.contains(this->queryLoc) && this->narrowestClassDefRange.contains(loc)) {
        // `loc` is contained in the current narrowestClassDefRange, and still contains `queryLoc`
        this->narrowestClassDefRange = loc;

        if (this->result().exists() && !loc.contains(this->result_.first)) {
            // If there's a result and it's not contained in the new narrowest range, we have to toss it out
            // (Method defs and class defs are not necessarily sorted by their locs)
            this->result_ = {core::Loc::none(), core::Symbols::noMethod()};
        }
    }

    this->scopeContainsQueryLoc.emplace_back(loc.contains(this->queryLoc));

    return tree;
}

ast::ExpressionPtr NextMethodFinder::postTransformClassDef(core::Context ctx, ast::ExpressionPtr tree) {
    ENFORCE(!this->scopeContainsQueryLoc.empty());
    this->scopeContainsQueryLoc.pop_back();

    return tree;
}

ast::ExpressionPtr NextMethodFinder::preTransformMethodDef(core::Context ctx, ast::ExpressionPtr tree) {
    auto &methodDef = ast::cast_tree_nonnull<ast::MethodDef>(tree);
    ENFORCE(methodDef.symbol.exists());
    ENFORCE(methodDef.symbol != core::Symbols::todoMethod());

    ENFORCE(!this->scopeContainsQueryLoc.empty());
    if (!this->scopeContainsQueryLoc.back()) {
        // Regardless of whether this method is after the queryLoc or inside the narrowestClassDefRange,
        // we're in a ClassDef whose scope doesn't contain the queryLoc.
        // (one case where this happens: nested Inner class)
        return tree;
    }

    auto currentMethod = methodDef.symbol;

    auto &currentMethodLocs = currentMethod.data(ctx)->locs();
    auto inFileOfQuery = [&](const auto &loc) { return loc.file() == this->queryLoc.file(); };
    auto maybeCurrentLoc = absl::c_find_if(currentMethodLocs, inFileOfQuery);
    if (maybeCurrentLoc == currentMethodLocs.end()) {
        return tree;
    }

    auto currentLoc = *maybeCurrentLoc;
    if (!currentLoc.exists()) {
        // Defensive in case location information is disabled (e.g., certain fuzzer modes)
        return tree;
    }

    ENFORCE(currentLoc.file() == this->queryLoc.file());
    ENFORCE(this->narrowestClassDefRange.exists());

    if (!this->narrowestClassDefRange.contains(currentLoc)) {
        // This method occurs outside the current narrowest range we know of for a ClassDef that
        // still contains queryLoc, so even if this MethodDef is after the queryLoc, it would not be
        // in the right scope.
        return tree;
    } else if (currentLoc.beginPos() < queryLoc.beginPos()) {
        // Current method is before query, not after.
        return tree;
    }

    // Current method starts at or after query loc. Starting 'at' is fine, because it can happen in cases like this:
    //   |def foo; end

    if (this->result().exists()) {
        // Method defs are not guaranteed to be sorted in order by their declLocs
        auto &resultLocs = this->result().data(ctx)->locs();
        auto maybeResultLoc = absl::c_find_if(resultLocs, inFileOfQuery);
        ENFORCE(maybeResultLoc != resultLocs.end(), "Must exist, because otherwise it wouldn't have been the result_");
        auto resultLoc = *maybeResultLoc;
        if (currentLoc.beginPos() < resultLoc.beginPos()) {
            // Found a method defined after the query but earlier than previous result: overwrite previous result
            this->result_ = {currentLoc, currentMethod};
            return tree;
        } else {
            // We've already found an earlier result, so the current is not the first
            return tree;
        }
    } else {
        // Haven't found a result yet, so this one is the best so far.
        this->result_ = {currentLoc, currentMethod};
        return tree;
    }
}

const core::MethodRef NextMethodFinder::result() const {
    return this->result_.second;
}

}; // namespace sorbet::realmain::lsp
