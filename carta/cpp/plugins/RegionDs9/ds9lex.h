/*
 * Adds extra functionality to the scanner.
 */
#ifndef DS9_DS9LEX_H_
#define DS9_DS9LEX_H_


// Flex expects the signature of yylex to be defined in the macro YY_DECL, and
// the C++ parser expects it to be declared. We can factor both as follows.
#ifndef YY_DECL
#define YY_DECL \
    yy::ds9parse::token_type ds9lex::lex( yy::ds9parse::semantic_type* yylval, yy::ds9parse::location_type* )
#endif


#ifndef __FLEX_LEXER_H
#define yyFlexLexer ds9lex_FlexLexer
#include <ds9FlexLexer.h>
#undef yyFlexLexer
#endif

#include <ds9parse.hpp>


// Scanner is a derived class to add some extra function to the scanner
// class. Flex itself creates a class named yyFlexLexer, which is renamed using
// macros to ds9lex_FlexLexer. However we change the context of the generated
// yylex() function to be contained within the ds9lex class. This is required
// because the yylex() defined in FlexLexer.has no parameters. */

class ds9lex : public ds9lex_FlexLexer {
public:
    // Create a new scanner object. The streams arg_yyin and arg_yyout default
    // to cin and cout, but that assignment is only made when initializing in
    // yylex().
    ds9lex( std::istream* yyin=0, std::ostream* yyout=0) : ds9lex_FlexLexer( yyin, yyout) {
        loc = new yy::ds9parse::location_type();
    }
    ~ds9lex( ) { }

    // This is the main lexing function. It is generated by flex according to
    // the macro declaration YY_DECL above. The generated bison parser then
    // calls this virtual function to fetch new tokens.

    virtual yy::ds9parse::token_type lex( yy::ds9parse::semantic_type* yylval,
            yy::ds9parse::location_type* location );

    void discard( int );
    void begin( int which, int doit );

private:
    yy::ds9parse::semantic_type* yylval = nullptr;
    yy::ds9parse::location_type* loc = nullptr;
};



class CallBack {
public:
	enum { SELECTCB, UNSELECTCB, HIGHLITECB, UNHIGHLITECB,
	       MOVEBEGINCB, MOVECB, MOVEENDCB, EDITBEGINCB,
	       EDITCB, EDITENDCB, ROTATEBEGINCB, ROTATECB,
	       ROTATEENDCB, DELETECB, TEXTCB, COLORCB,
	       LINEWIDTHCB, PROPERTYCB, FONTCB, KEYCB,
	       UPDATECB
	     };
};

class Marker {
public:
	// Select-- user may select the marker
	// Highlite-- user may highlite the marker
	// Edit-- user may edit the marker
	// Move-- user may move the marker
	// Rotate-- user may rotate the marker
	// Delete-- user may delete the marker
	// Fixed-- marker is fixed in size (not scaled based on zoom)
	// Include-- marker is 'included' or 'excluded' ie '+' or '-'
	// Source-- marker is a 'source' or 'background' marker
	// Dash-- render with dashed line
	enum Property { NONE=0, SELECT=1, HIGHLITE=2, EDIT=4, MOVE=8, ROTATE=16,
	                DELETE=32, FIXED=64, INCLUDE=128, SOURCE=256, DASH=512
	              };
};
#endif


