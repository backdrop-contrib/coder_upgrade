<?php declare(strict_types=1);


use PhpParser\Node;
use PhpParser\Node\Expr;
use PhpParser\Node\Expr\AssignOp;
use PhpParser\Node\Expr\BinaryOp;
use PhpParser\Node\Expr\Cast;
use PhpParser\Node\Name;
use PhpParser\Node\Scalar;
use PhpParser\Node\Scalar\MagicConst;
use PhpParser\Node\Stmt;
use PhpParser\PrettyPrinterAbstract;
use PhpParser\PrettyPrinter\Standard;

class CoderUpgradePrettyPrinterStandard extends Standard
{

    /**
     * Overrides to change array items to separate lines.
     */
    protected function pExpr_Array(Expr\Array_ $node) {
        $syntax = $node->getAttribute('kind',
            $this->options['shortArraySyntax'] ? Expr\Array_::KIND_SHORT : Expr\Array_::KIND_LONG);
        if ($syntax === Expr\Array_::KIND_SHORT) {
            return '[' . $this->pCommaSeparatedMultiline($node->items, true) . ']';
        } else {
            return 'array(' . $this->pCommaSeparatedMultiline($node->items, true) . ')';
        }
    }

    /**
     * Overrides to remove newline after function name.
     */
    protected function pStmt_Function(Stmt\Function_ $node) {
        return $this->pAttrGroups($node->attrGroups)
             . 'function ' . ($node->byRef ? '&' : '') . $node->name
             . '(' . $this->pCommaSeparated($node->params) . ')'
             . (null !== $node->returnType ? ' : ' . $this->p($node->returnType) : '')
             . ' {' . $this->pStmts($node->stmts) . $this->nl . '}';
    }

    /**
     * Overrides to change indentation level to 2.
     */
    protected function indent() {
        $this->indentLevel += 2;
        $this->nl .= '  ';
    }

    /**
     * Overrides to change indentation level to 2.
     */
    protected function outdent() {
        assert($this->indentLevel >= 2);
        $this->indentLevel -= 2;
        $this->nl = "\n" . str_repeat(' ', $this->indentLevel);
    }
}
