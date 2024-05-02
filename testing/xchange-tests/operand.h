/*
 * $Id: operand.h,v 1.13 2005/09/15 20:51:24 lofstead Exp $
 */
#ifndef OPERAND_H
#define OPERAND_H

#include <vector>
#include <sstream>

/*
 * There are three main structs in this file:
 * operand - the parent class of all transformation pieces
 * element - the particular type of operand representing a data element
 * operation - the representation of the source and destination halves
 *       of a transformation operation.  The actual specifics of the
 *       type of the operation is encoded in the src element by using
 *       a class derived from operand.  The destination element is
 *       specifically limited to just the element type making it a
 *       valid LHS for an assignment operation.
 *
 * Please refer to the individual structs below for more details.
 *
 * Extending the system:
 * The current process for extending the system is not completely
 * transparent for the plug-in writer.  It is, however, simple and
 * relatively painless.  Below are the steps needed and where
 * changes need to be made:
 * 1. create a new type derived from operand to represent the
 *    transformation operation.  Create the toString to generate
 *    proper ECL code for that particular piece of an operand.  The
 *    code necessary for loops and such.
 * 2. create a function that will parse the new tag structure and
 *    populate your new object returning an operand *.
 * 3. in xml.cc update parseOperand to recognize the new tag name
 *    and call the new function.
 *
 * Later changes into a more class oriented structure may occur
 * making it a little more transparent.  At that point, there would
 * need to be a registration mechanism for plug-ins and similar
 * kinds of infrastructure.  That was deemed more trouble than it was
 * worth for this stage of the project.
 */

/*
 * All operands in a transform are derived from this base class. The
 * toString is used to generate ECL code currently.  A rework of this to
 * generate different languages may happen in the future.
 */
struct operand
{
    virtual ~operand () {}

    // generate the code for this operand.  The prefix is provided for
    // proper generation of the "input." and "output." prefix on each
    // element reference.  Since it is not needed for all possible
    // output scenarios, it is made optional.
    virtual string toString (const char * prefix = NULL) = 0;

    // return a list of operands representing the referenced elements
    // caller owns the returned list
    virtual vector <struct element *> getSrcElements () = 0;

    virtual bool operator == (operand & rhs) = 0;

    virtual operand * newCopy () = 0;
};

/*
 * The type representing an element reference.  This can be used for both
 * source and destination elements or embedded in other operands to build
 * a more complex structure.  This is a recursive data structure with each
 * nested level representing a nesting level in the type hierarchy.
 */
struct element : operand
{
    element () : name (0), subelement (0), subscript (" "), src (DEFAULT) {}
    element (element * orig) : name (0), subelement (0), subscript (" "), src (DEFAULT)
        {if (orig) {name = strdup (orig->name);
                    if (orig->subelement)
                        subelement = new element (orig->subelement);
                    subscript = orig->subscript;
                    src = orig->src;
                   }
        }
    ~element () {delete name; delete subelement;}

    char * name;  // name of this component of the element.
    string subscript;  // the subscript used when refering to the element
                       // this is used for code generation.
    struct element * subelement; // The sub-element or NULL
    enum SOURCE {DEFAULT, SRC, DST} src;

    operand * newCopy ()
    {
        element * e = new element (this);

        return e;
    }

    // generate the ECL code for this element
    string toString (const char * prefix = NULL)
    {
        string s;

        element * e = subelement;
        if (prefix)
        {
            s += prefix;
        }
        s += name;
        if (subscript != " ")
        {
            s.append (" [");
            s += subscript;
            s.append ("]");
        }

        while (e)
        {
            s.append (".").append (e->name);
            if (e->subscript != " ")
            {
                s.append (" [");
                s += e->subscript;
                s.append ("]");
            }

            e = e->subelement;
        }

        return s;
    }

    vector <element *> getSrcElements ()
    {
        vector <element *> ops;
        ops.push_back (new element (this));

        return ops;
    }

    // add a new subelement to the deepest nesting level
    void addSubelement (element * e)
    {
        element * root = this;

        while (root->subelement)
            root = root->subelement;

        root->subelement = e;
    }

    bool operator == (operand & rhs)
    {
        try
        {
            element & r = dynamic_cast <element &> (rhs);

            return *this == r;
        }
        catch (...)
        {
            return false;
        }
    }

    // compare two elements
    bool operator == (element & rhs)
    {
        if (   !strcmp (name, rhs.name)
            // && subscript == rhs.subscript // why check this?
            && src == rhs.src
           )
        {
            if (subelement && rhs.subelement)
            {
                return (*subelement == *rhs.subelement);
            }
            else
            {
               if (!subelement && !rhs.subelement)
               {
                   return true;
               }
               else
               {
                   return false;
               }
            }
        }
    }
};

/*
 * The type representing a math operation.  These operations conform to the
 * MathML standard with the <ci> tags being used to enclose element references.
 * The contents of the <ci> tag can either be a top level element name or a
 * <compound> structure that will be parsed into an element with sub-elements.
 */
struct math : operand
{
    math () : token (0), identifier (0), next (0) {}
    ~math () {delete token; delete identifier; delete next;}

    char * token;  // The token representing this piece of the math expression
    element * identifier; // The element referenced
    struct math * next;  // the next item in the expression

    operand * newCopy ()
    {
        math * m = new math ();

        if (token)
        {
            m->token = new char [strlen (token) + 1];
            strcpy (m->token, token);
        }
        else
        {
            m->token = NULL;
        }

        if (identifier)
        {
            m->identifier = (element *) identifier->newCopy ();
        }
        else
        {
            m->identifier = NULL;
        }

        if (next)
        {
            m->next = (math *) next->newCopy ();
        }
        else
        {
            m->next = NULL;
        }

        return m;
    }

    // generate the ECL code for this operation
    string toString (const char * prefix = NULL)
    {
        string s;

        for (math * m = this; m; m = m->next)
        {
            if (m->token)
            {
                s += m->token;
            }
            else
            {
                if (m->identifier)
                {
                    s += m->identifier->toString (prefix);
                }
                else
                {
                    cerr << "ERROR: math has neither token nor identifier"
                         << endl;
                }
            }
        }

        return s;
    }

    vector <element *> getSrcElements ()
    {
        vector <element *> ops;

        for (math * m = this; m; m = m->next)
        {
            // only take the elements and not the other textual components
            if (m->identifier)
            {
                ops.push_back (new element (m->identifier));
            }
        }

        return ops;
    }

    bool operator == (operand & rhs)
    {
        try
        {
            math & r = dynamic_cast <math &> (rhs);

            return *this == r;
        }
        catch (...)
        {
            return false;
        }
    }

    bool operator == (math & rhs)
    {
        if (   (   (token && rhs.token && (!strcmp (token, rhs.token)))
                || (!token && !rhs.token)
               )
            && (   (identifier && rhs.identifier && (*identifier == *rhs.identifier))
                || (!identifier && !rhs.identifier)
               )
            && (   (next && rhs.next && (*next == *rhs.next))
                || (!next && !rhs.next)
               )
           )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

/*
 * The type representing a reduction operation.  This is the NxN => 1 mapping
 * typically done to scale down an image or related data set.
 */
struct reduce : operand
{
    reduce () : identifier (0), factor (0) {}
    ~reduce () {delete identifier;}

    element * identifier; // source element
    int factor; // dimension of the area to reduce to a single element.  This
                // specifies the depth to work from when doing the operation.

    operand * newCopy ()
    {
        reduce * r = new reduce ();

        if (identifier)
        {
            r->identifier = (element *) identifier->newCopy ();
        }
        else
        {
            r->identifier = NULL;
        }

        r->factor = factor;

        return r;
    }

    // generate the ECL code for this operation
    string toString (const char * prefix = NULL)
    {
        ostringstream s;
        char buffer [4];
        sprintf (buffer, "%d", factor * factor);

        first = true;
        s << "(";
        s << generateTerms (prefix, identifier, identifier);
        s << ") / " << buffer;

        return s.str ();
    }

    vector <element *> getSrcElements ()
    {
        vector <element *> ops;
        ops.push_back (new element (identifier));

        return ops;
    }

    // private helpers for the above code
    private:
    mutable bool first; // used for generateTerms;
    // generate the terms recursively.
    string generateTerms(const char * prefix, element * root, element * current)
    {
        if (current == NULL)
        {
            string s = (first ? "" : " + ");
            first = false;
            return s + (prefix == NULL ? "" : prefix) + root->toString ();
        }

        char var = current->subscript [0];
        string terms;
        for (int i = 0; i < factor; i++)
        {
            if (var != ' ')
            {
            char buffer [3];
            sprintf (buffer, "%d", factor);
            current->subscript = buffer;
            current->subscript += "*";
            current->subscript += var;
            if (i != 0)
            {
                current->subscript += "+";
                sprintf (buffer, "%d", i);
                current->subscript += buffer;
            }
            }

            terms += generateTerms (prefix, root, current->subelement);
            current->subscript = var;
        }

        return terms;
    }

    bool operator == (operand & rhs)
    {
        try
        {
            reduce & r = dynamic_cast <reduce &> (rhs);

            return *this == r;
        }
        catch (...)
        {
            return false;
        }
    }

    bool operator == (reduce & rhs)
    {
        if (   (factor == rhs.factor)
            && (   (identifier && rhs.identifier && (*identifier == *rhs.identifier))
                || (!identifier && !rhs.identifier)
               )
           )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

/*
 * A transformation for an XML file consists of a series of operations.  Each
 * operation consists of a destination element and a source operand.  The
 * source operand is a subclass of operand representing the kind of operation
 * that needs to be performed to generate the value for the destination item.
 * Each transformation operation is kept in this linked list.
 */
struct operation
{
    struct element * dst;
    struct operand * src;
    struct operation * next;

    xmlNodePtr transform;  // needed for passing on the XML for intermediate nodes

    operation () : dst (0), src (0), transform (0), next (0) {}
    ~operation () {delete dst; delete src; if (transform) xmlFreeNode (transform); delete next;}

    bool operator == (struct operation & rhs) const
    {
        if (   (   (dst && rhs.dst && (*dst == *rhs.dst))
                || (!dst && !rhs.dst)
               )
            && (   (src && rhs.src && (*src == *rhs.src))
                || (!src && !rhs.src)
               )
            && (   (next && rhs.next && (*next == *rhs.next))
                || (!next && !rhs.next)
               )
           )
        {
            return true;
        }

        return false;
    }

    string toString () const
    {
        string n;

        if (next)
        {
            n = next->toString ();
        }

        return "dst:\n" + dst->toString () + "\nsrc:\n" + src->toString () + n + "\n";
    }

    operation * newCopy ()
    {
        operation * op = new operation ();

        if (dst)
        {
            op->dst = (element *) dst->newCopy ();
        }
        else
        {
            op->dst = NULL;
        }

        if (src)
        {
            op->src = src->newCopy ();
        }
        else
        {
            op->src = NULL;
        }

        if (next)
        {
            op->next = next->newCopy ();
        }
        else
        {
            op->next = NULL;
        }

        return op;
    }
};
#endif
/*
 * $Log: operand.h,v $
 * Revision 1.13  2005/09/15 20:51:24  lofstead
 * code cleanup (no functional changes)
 *
 */
