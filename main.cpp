#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <variant>
#include <limits>
#include <cmath>

enum TokenCode{
    NONE_TOK = -1,
    EOS_TOK = 0,

    NUM_TOK = 256,
    ID_TOK = 257,

    // Keywords
    IF_TOK = 259,
    ELSE_TOK = 261,
    WHILE_TOK = 262,
    INPUT_TOK = 264,
    OUTPUT_TOK = 265,
    INT_TOK = 266,  // Тип 'int' (для объявления)
    IMAS_TOK = 267, // Тип 'arr' (для объявления массива, 'imas' было в исходных #define)
    SIN_TOK = 274,
    COS_TOK = 275,
    TG_TOK = 276,
    CTG_TOK = 277,

    EOF_TOK = 271,
    BEG_TOK = 272,
    END_TOK = 273,

    // Operators and Punctuation
    PLUS_TOK,       // +
    MINUS_TOK,      // -
    STAR_TOK,       // *
    SLASH_TOK,      // /
    EQ_TOK,         // = (присваивание)
    EQ_COMPARE_TOK, // ~ (сравнение на равенство)
    GT_TOK,         // >
    LT_TOK,         // <
    NOT_TOK,        // ! (в вашей грамматике D -> !G, вероятно, "не равно")
    LPAREN_TOK,     // (
    RPAREN_TOK,     // )
    LBRACKET_TOK,   // [
    RBRACKET_TOK,   // ]
    SEMICOLON_TOK,  // ;
    DOLLAR_TOK,     // $ (если используется как терминальный символ конца программы или чего-то еще)

    // Special for lexer
    ERROR_TOK,
    SPACE_TOK,  // Обычно пропускается, но может быть токеном для отладки
    NEWLINE_TOK // Обычно пропускается или влияет на номер строки
};

//  Token Structure
struct Token
{
    TokenCode code;
    std::string lexeme;
    int line;

    Token(TokenCode c = NONE_TOK, std::string l = "", int ln = 0) : code(c), lexeme(std::move(l)), line(ln) {}

    std::string codeToString() const
    {
        switch (code)
        {
        case NONE_TOK:
            return "NONE_TOK";
        case EOS_TOK:
            return "EOS_TOK";
        case NUM_TOK:
            return "NUM_TOK";
        case ID_TOK:
            return "ID_TOK";
        case IF_TOK:
            return "IF_TOK";
        case ELSE_TOK:
            return "ELSE_TOK";
        case WHILE_TOK:
            return "WHILE_TOK";
        case INPUT_TOK:
            return "INPUT_TOK";
        case OUTPUT_TOK:
            return "OUTPUT_TOK";
        case INT_TOK:
            return "INT_TOK";
        case IMAS_TOK:
            return "IMAS_TOK";
        case EOF_TOK:
            return "EOF_TOK";
        case BEG_TOK:
            return "BEG_TOK";
        case END_TOK:
            return "END_TOK";
        case PLUS_TOK:
            return "PLUS_TOK";
        case MINUS_TOK:
            return "MINUS_TOK";
        case STAR_TOK:
            return "STAR_TOK";
        case SLASH_TOK:
            return "SLASH_TOK";
        case EQ_TOK:
            return "EQ_TOK";
        case EQ_COMPARE_TOK:
            return "EQ_COMPARE_TOK";
        case GT_TOK:
            return "GT_TOK";
        case LT_TOK:
            return "LT_TOK";
        case NOT_TOK:
            return "NOT_TOK";
        case LPAREN_TOK:
            return "LPAREN_TOK";
        case RPAREN_TOK:
            return "RPAREN_TOK";
        case LBRACKET_TOK:
            return "LBRACKET_TOK";
        case RBRACKET_TOK:
            return "RBRACKET_TOK";
        case SEMICOLON_TOK:
            return "SEMICOLON_TOK";
        case DOLLAR_TOK:
            return "DOLLAR_TOK";
        case ERROR_TOK:
            return "ERROR_TOK";
        case SPACE_TOK:
            return "SPACE_TOK";
        case NEWLINE_TOK:
            return "NEWLINE_TOK";
        default:
            return "UNKNOWN_TOKEN_CODE(" + std::to_string(static_cast<int>(code)) + ")";
        }
    }
};

// Lexer Configuration & Globals
#define LEXER_BUFFER_SIZE 1024

const int S_STATE = 0;
const int A_STATE = 1;
const int B_STATE = 2;

// Character categories
const int CAT_LETTER = 0;
const int CAT_DIGIT = 1;
const int CAT_PLUS = 2;
const int CAT_MINUS = 3;
const int CAT_EQ = 4; // =
const int CAT_STAR = 5;
const int CAT_SLASH = 6;
const int CAT_SPACE = 7;
const int CAT_LPAREN = 8;
const int CAT_RPAREN = 9;
const int CAT_LBRACKET = 10;
const int CAT_RBRACKET = 11;
const int CAT_GT = 12;
const int CAT_LT = 13;
const int CAT_NOT = 14; // !
const int CAT_SEMICOLON = 15;
const int CAT_NEWLINE = 16;
const int CAT_DOLLAR = 17;
const int CAT_TILDE = 18; // ~ (для сравнения на равенство)
const int CAT_OTHER = 19;
const int NUM_CHAR_CATEGORIES = 20;

int lexTable[3][NUM_CHAR_CATEGORIES];
int AsciiTable[128];
std::map<std::string, TokenCode> keywords;

// Initialize AsciiTable
void initialize_lexer_tables()
{

    for (int i = 0; i < 128; ++i)
    {
        AsciiTable[i] = CAT_OTHER;
    }
    for (char c = 'a'; c <= 'z'; ++c)
        AsciiTable[static_cast<unsigned char>(c)] = CAT_LETTER;
    for (char c = 'A'; c <= 'Z'; ++c)
        AsciiTable[static_cast<unsigned char>(c)] = CAT_LETTER;
    for (char c = '0'; c <= '9'; ++c)
        AsciiTable[static_cast<unsigned char>(c)] = CAT_DIGIT;

    AsciiTable[static_cast<unsigned char>('+')] = CAT_PLUS;
    AsciiTable[static_cast<unsigned char>('-')] = CAT_MINUS;
    AsciiTable[static_cast<unsigned char>('=')] = CAT_EQ;
    AsciiTable[static_cast<unsigned char>('*')] = CAT_STAR;
    AsciiTable[static_cast<unsigned char>('/')] = CAT_SLASH;
    AsciiTable[static_cast<unsigned char>(' ')] = CAT_SPACE;
    AsciiTable[static_cast<unsigned char>('(')] = CAT_LPAREN;
    AsciiTable[static_cast<unsigned char>(')')] = CAT_RPAREN;
    AsciiTable[static_cast<unsigned char>('[')] = CAT_LBRACKET;
    AsciiTable[static_cast<unsigned char>(']')] = CAT_RBRACKET;
    AsciiTable[static_cast<unsigned char>('>')] = CAT_GT;
    AsciiTable[static_cast<unsigned char>('<')] = CAT_LT;
    AsciiTable[static_cast<unsigned char>('!')] = CAT_NOT;
    AsciiTable[static_cast<unsigned char>(';')] = CAT_SEMICOLON;
    AsciiTable[static_cast<unsigned char>('\n')] = CAT_NEWLINE;
    AsciiTable[static_cast<unsigned char>('$')] = CAT_DOLLAR;
    AsciiTable[static_cast<unsigned char>('~')] = CAT_TILDE;

    // Initialize lexTable (Таблица переходов -> Семантические программы)
    // Семантические программы:
    // 1: Начать идентификатор/ключ.слово
    // 2: Начать число
    // 3: '+'
    // 4: '-'
    // 5: '=' (присваивание)
    // 6: '*'
    // 7: '/'
    // 8: Пробел (пропустить)
    // 9: '('
    // 10: ')'
    // 11: '['
    // 12: ']'
    // 13: '>'
    // 14: '<'
    // 15: '!' (для !=)
    // 16: ';'
    // 18: '\n' (новая строка)
    // 19: Ошибка в S_STATE
    // 20: '$'
    // 21: Продолжить идентификатор/ключ.слово
    // 22: Завершить идентификатор/ключ.слово (посмотреть в keywords)
    // 24: Ошибка в A_STATE или B_STATE (недопустимый символ после начала)
    // 27: Продолжить число
    // 28: Завершить число
    // 30: '~' (сравнение на равенство) - НОВАЯ СЕМАНТИЧЕСКАЯ ПРОГРАММА

    //          L  D  +  -  =  *  / sp  (  )  [  ]  >  <  !  ; \n  $  ~  dr <- Char Category
    // S_STATE
    lexTable[S_STATE][CAT_LETTER] = 1;
    lexTable[S_STATE][CAT_DIGIT] = 2;
    lexTable[S_STATE][CAT_PLUS] = 3;
    lexTable[S_STATE][CAT_MINUS] = 4;
    lexTable[S_STATE][CAT_EQ] = 5;
    lexTable[S_STATE][CAT_STAR] = 6;
    lexTable[S_STATE][CAT_SLASH] = 7;
    lexTable[S_STATE][CAT_SPACE] = 8;
    lexTable[S_STATE][CAT_LPAREN] = 9;
    lexTable[S_STATE][CAT_RPAREN] = 10;
    lexTable[S_STATE][CAT_LBRACKET] = 11;
    lexTable[S_STATE][CAT_RBRACKET] = 12;
    lexTable[S_STATE][CAT_GT] = 13;
    lexTable[S_STATE][CAT_LT] = 14;
    lexTable[S_STATE][CAT_NOT] = 15;
    lexTable[S_STATE][CAT_SEMICOLON] = 16;
    lexTable[S_STATE][CAT_NEWLINE] = 18;
    lexTable[S_STATE][CAT_DOLLAR] = 20;
    lexTable[S_STATE][CAT_TILDE] = 30; // Новое для ~
    lexTable[S_STATE][CAT_OTHER] = 19;

    // A_STATE (Идентификатор/ключевое слово)
    for (int i = 0; i < NUM_CHAR_CATEGORIES; ++i)
        lexTable[A_STATE][i] = 22;      // По умолчанию завершить
    lexTable[A_STATE][CAT_LETTER] = 21; // Продолжить буквой
    lexTable[A_STATE][CAT_DIGIT] = 21;  // Продолжить цифрой
    lexTable[A_STATE][CAT_OTHER] = 24;  // Ошибка, если другой символ

    // B_STATE (Число)
    for (int i = 0; i < NUM_CHAR_CATEGORIES; ++i)
        lexTable[B_STATE][i] = 28;      // По умолчанию завершить
    lexTable[B_STATE][CAT_DIGIT] = 27;  // Продолжить цифрой
    lexTable[B_STATE][CAT_OTHER] = 24;  // Ошибка, если другой символ (кроме тех, что завершают)
    lexTable[B_STATE][CAT_LETTER] = 24; // Буква после числа - ошибка

    // Initialize keywords
    keywords["if"] = IF_TOK;
    keywords["else"] = ELSE_TOK;
    keywords["while"] = WHILE_TOK;
    keywords["input"] = INPUT_TOK;   // грамматика использует 'cin'
    keywords["output"] = OUTPUT_TOK; // грамматика использует 'cout'
    keywords["int"] = INT_TOK;
    keywords["arr"] = IMAS_TOK; // 'arr' для объявления массива, соответствует IMAS_TOK
    keywords["begin"] = BEG_TOK;
    keywords["end"] = END_TOK;
    keywords["sin"] = SIN_TOK;
    keywords["cos"] = COS_TOK;
    keywords["tg"] = TG_TOK;
    keywords["ctg"] = CTG_TOK;
}

// Lexer Class
class Lexer
{
public:
    Lexer(std::istream &in_stream) : input_stream(in_stream), current_line(1), char_buffer_valid(false)
    {
        static bool tables_initialized = false;
        if (!tables_initialized)
        {
            initialize_lexer_tables();
            tables_initialized = true;
        }
    }

    Token getNextToken()
    {
        std::string current_lexeme = "";
        int currentState = S_STATE;
        int token_start_line = current_line;

        while (true)
        {
            int ch_int = get_char();
            char c = static_cast<char>(ch_int);

            if (ch_int == EOF)
            {
                if (!current_lexeme.empty())
                {
                    if (currentState == A_STATE)
                        return finalize_identifier(current_lexeme, token_start_line);
                    if (currentState == B_STATE)
                        return finalize_number(current_lexeme, token_start_line);
                }
                return Token(EOF_TOK, "EOF", current_line);
            }

            int char_category = (ch_int < 0 || ch_int > 127) ? CAT_OTHER : AsciiTable[ch_int];
            int semantic_action = lexTable[currentState][char_category];

            switch (semantic_action)
            {
            case 1:
                current_lexeme += c;
                currentState = A_STATE;
                token_start_line = current_line;
                break;
            case 2:
                current_lexeme += c;
                currentState = B_STATE;
                token_start_line = current_line;
                break;
            case 3:
                return Token(PLUS_TOK, std::string(1, c), current_line);
            case 4:
                return Token(MINUS_TOK, std::string(1, c), current_line);
            case 5:
                return Token(EQ_TOK, std::string(1, c), current_line);
            case 6:
                return Token(STAR_TOK, std::string(1, c), current_line);
            case 7:
                return Token(SLASH_TOK, std::string(1, c), current_line);
            case 8:
                currentState = S_STATE;
                current_lexeme = "";
                break; // Пробел
            case 9:
                return Token(LPAREN_TOK, std::string(1, c), current_line);
            case 10:
                return Token(RPAREN_TOK, std::string(1, c), current_line);
            case 11:
                return Token(LBRACKET_TOK, std::string(1, c), current_line);
            case 12:
                return Token(RBRACKET_TOK, std::string(1, c), current_line);
            case 13:
                return Token(GT_TOK, std::string(1, c), current_line);
            case 14:
                return Token(LT_TOK, std::string(1, c), current_line);
            case 15:
                return Token(NOT_TOK, std::string(1, c), current_line); // !
            case 16:
                return Token(SEMICOLON_TOK, std::string(1, c), current_line);
            case 18:
                current_line++;
                currentState = S_STATE;
                current_lexeme = "";
                break; // \n
            case 19:   // Ошибка в S_STATE
                std::cerr << "Lexical Error (Line " << current_line << "): Invalid character '" << c << "' in initial state." << std::endl;
                current_lexeme = "";
                currentState = S_STATE; // Пропустить символ и сбросить
                break;
            case 20:
                return Token(DOLLAR_TOK, std::string(1, c), current_line);
            case 21: // Продолжить идентификатор/ключ.слово
                if (current_lexeme.length() < LEXER_BUFFER_SIZE - 1)
                {
                    current_lexeme += c;
                }
                else
                {
                    std::cerr << "Lexical Error (Line " << token_start_line << "): Identifier too long: " << current_lexeme << "..." << std::endl;
                    unget_char(c);
                    return finalize_identifier(current_lexeme, token_start_line);
                }
                break;
            case 22: // Завершить идентификатор/ключ.слово
                unget_char(c);
                return finalize_identifier(current_lexeme, token_start_line);
            case 24: // Ошибка в A_STATE или B_STATE
                unget_char(c);
                std::cerr << "Lexical Error (Line " << token_start_line << "): Invalid character '" << c << "' after '" << current_lexeme << "'" << std::endl;
                if (!current_lexeme.empty())
                {
                    if (currentState == A_STATE)
                    {
                        return finalize_identifier(current_lexeme, token_start_line);
                    }
                    else if (currentState == B_STATE)
                    {
                        return finalize_number(current_lexeme, token_start_line);
                    }
                }
                return Token(ERROR_TOK, current_lexeme + c, token_start_line);
            case 27: // Продолжить число
                if (current_lexeme.length() < LEXER_BUFFER_SIZE - 1)
                {
                    current_lexeme += c;
                }
                else
                {
                    std::cerr << "Lexical Error (Line " << token_start_line << "): Number too long: " << current_lexeme << "..." << std::endl;
                    unget_char(c);
                    return finalize_number(current_lexeme, token_start_line);
                }
                break;
            case 28: // Завершить число
                unget_char(c);
                return finalize_number(current_lexeme, token_start_line);
            case 30:
                return Token(EQ_COMPARE_TOK, std::string(1, c), current_line); // Сравнение ~

            default:
                std::cerr << "Lexical Error (Line " << current_line << "): Unknown semantic action " << semantic_action
                          << " for char '" << c << "' (cat " << char_category << ") in state " << currentState << std::endl;
                return Token(ERROR_TOK, std::string(1, c), current_line);
            }
        }
    }

private:
    std::istream &input_stream;
    int current_line;
    char char_buffer;
    bool char_buffer_valid;

    int get_char()
    {
        if (char_buffer_valid)
        {
            char_buffer_valid = false;
            return char_buffer;
        }
        return input_stream.get();
    }

    void unget_char(char c)
    {
        char_buffer = c;
        char_buffer_valid = true;
    }

    Token finalize_identifier(const std::string &lexeme, int line_num)
    {
        auto it = keywords.find(lexeme);
        if (it != keywords.end())
        {
            return Token(it->second, lexeme, line_num);
        }
        return Token(ID_TOK, lexeme, line_num);
    }

    Token finalize_number(const std::string &lexeme, int line_num)
    {
        return Token(NUM_TOK, lexeme, line_num);
    }
};

// RPN (OPS) Generator Structures
enum class RPNItemType
{
    VAR,
    ARRAY_BASE,
    CONST,
    OPERATION,
    LABEL_DEF,
    JUMP,
    JUMP_FALSE,
    ARRAY_ACCESS,
    ARRAY_ASSIGN,
    INPUT,
    OUTPUT,
    FUNC_SIN,
    FUNC_COS,
    FUNC_TG,
    FUNC_CTG
};

struct RPNEntry
{
    RPNItemType type;
    std::string value;
    int line_num; // Line number from source for error reporting

    RPNEntry(RPNItemType t, std::string val, int line) : type(t), value(std::move(val)), line_num(line) {}

    std::string typeToString() const
    {
        switch (type)
        {
        case RPNItemType::VAR:
            return "VAR";
        case RPNItemType::ARRAY_BASE:
            return "ARRAY_BASE";
        case RPNItemType::CONST:
            return "CONST";
        case RPNItemType::OPERATION:
            return "OPERATION";
        case RPNItemType::LABEL_DEF:
            return "LABEL_DEF";
        case RPNItemType::JUMP:
            return "JUMP";
        case RPNItemType::JUMP_FALSE:
            return "JUMP_FALSE";
        case RPNItemType::ARRAY_ACCESS:
            return "ARRAY_ACCESS_OP"; // "[]"
        case RPNItemType::ARRAY_ASSIGN:
            return "ARRAY_ASSIGN_OP"; // "[]=" (Actually handled by OPERATION "[]=")
        case RPNItemType::INPUT:
            return "INPUT_OP";
        case RPNItemType::OUTPUT:
            return "OUTPUT_OP";
        case RPNItemType::FUNC_SIN:
            return "FUNC_SIN";
        case RPNItemType::FUNC_COS:
            return "FUNC_COS";
        case RPNItemType::FUNC_TG:
            return "FUNC_TG";
        case RPNItemType::FUNC_CTG:
            return "FUNC_CTG";
        default:
            return "UNKNOWN_RPN_TYPE";
        }
    }
};

enum class SymbolClass
{
    UNKNOWN,
    INT_VAR,
    INT_ARRAY
};

struct SymbolInfo
{
    SymbolClass s_class = SymbolClass::UNKNOWN;
    TokenCode type_token = NONE_TOK;
    int size = 0; // For arrays
    int declaration_line = 0;
    bool is_declared = false;
};

// --- RPN Generator Class ---
class RPNGenerator
{
public:
    RPNGenerator(const std::vector<Token> &tokens)
        : m_tokens(tokens), m_currentIndex(0), m_labelCounter(0) {}

    std::vector<RPNEntry> generate()
    {
        m_rpn.clear();
        m_symbolTable.clear();
        m_currentIndex = 0;
        m_labelCounter = 0;
        if (m_tokens.empty() || m_tokens.back().code != EOF_TOK)
        {
            throw std::runtime_error("Parser Error: Token stream is empty or does not end with EOF_TOK.");
        }
        parse_P();
        if (currentToken().code != EOF_TOK)
        {
            throwError("Expected end of program (EOF_TOK) but found " + currentToken().codeToString() + " ('" + currentToken().lexeme + "')");
        }
        return m_rpn;
    }
    const std::map<std::string, SymbolInfo> &getSymbolTable() const { return m_symbolTable; }

private:
    const std::vector<Token> &m_tokens;
    size_t m_currentIndex;
    std::vector<RPNEntry> m_rpn;
    std::map<std::string, SymbolInfo> m_symbolTable;
    int m_labelCounter;

    Token currentToken()
    {
        if (m_currentIndex < m_tokens.size())
            return m_tokens[m_currentIndex];
        if (!m_tokens.empty() && m_tokens.back().code == EOF_TOK)
            return m_tokens.back();
        throw std::runtime_error("Parser Error: Unexpected end of token stream (currentToken).");
    }
    Token consumeToken()
    {
        if (m_currentIndex < m_tokens.size())
        {
            if (m_tokens[m_currentIndex].code == EOF_TOK && (m_currentIndex + 1 < m_tokens.size()))
            {

                throw std::runtime_error("Parser Error: Attempt to consume EOF_TOK when more tokens exist.");
            }
            return m_tokens[m_currentIndex++];
        }

        throw std::runtime_error("Parser Error: Unexpected end of token stream (consumeToken).");
    }
    Token expect(TokenCode expectedCode, const std::string &errorMessagePrefix)
    {
        Token t = consumeToken();
        if (t.code != expectedCode)
        {
            Token tempExpected(expectedCode, "");
            throwError(errorMessagePrefix + ". Expected " + tempExpected.codeToString() +
                       " but got " + t.codeToString() + " ('" + t.lexeme + "')");
        }
        return t;
    }
    void throwError(const std::string &message)
    {
        int line = 0;
        if (m_currentIndex < m_tokens.size())
        {
            line = m_tokens[m_currentIndex].line;
        }
        else if (m_currentIndex > 0 && m_currentIndex <= m_tokens.size())
        {
            line = m_tokens[m_currentIndex - 1].line;
        }
        throw std::runtime_error("Syntax Error (Line " + std::to_string(line) + "): " + message);
    }
    std::string newLabel()
    {
        std::stringstream ss;
        ss << "L" << m_labelCounter++;
        return ss.str();
    }

    void addSymbol(const std::string &name, SymbolClass s_class, TokenCode type, int line, int arr_size = 0)
    {
        if (m_symbolTable.count(name) && m_symbolTable[name].is_declared)
        {
            throwError("Identifier '" + name + "' already declared at line " + std::to_string(m_symbolTable[name].declaration_line) + ".");
        }
        m_symbolTable[name] = {s_class, type, arr_size, line, true};
    }
    SymbolInfo getSymbol(const std::string &name, int use_line)
    {
        auto it = m_symbolTable.find(name);
        if (it == m_symbolTable.end() || !it->second.is_declared)
        {
            throwError("Undeclared identifier '" + name + "' used at line " + std::to_string(use_line) + ".");
        }
        return it->second;
    }

    // P → int LE | arr ME | begin A end
    void parse_P()
    {
        TokenCode tc = currentToken().code;
        if (tc == INT_TOK)
            parse_int_LE();
        else if (tc == IMAS_TOK)
            parse_arr_ME();
        else if (tc == BEG_TOK)
        {
            consumeToken();
            parse_A();
            expect(END_TOK, "program block");
        }
        else
            throwError("Program must start with 'int', 'arr', or 'begin'. Found " + currentToken().codeToString());
    }

    // E → int LE | arr ME | begin A end | λ
    void parse_E()
    {
        TokenCode tc = currentToken().code;
        if (tc == INT_TOK)
            parse_int_LE();
        else if (tc == IMAS_TOK)
            parse_arr_ME();
        else if (tc == BEG_TOK)
        {
            consumeToken();
            parse_A();
            expect(END_TOK, "block in E");
        }
        // λ case: do nothing, currentToken() will be END_TOK or EOF_TOK or similar, handled by caller
    }

    void parse_int_LE()
    { // int a; E
        expect(INT_TOK, "int declaration");
        Token id = expect(ID_TOK, "identifier after 'int'");
        addSymbol(id.lexeme, SymbolClass::INT_VAR, INT_TOK, id.line);
        expect(SEMICOLON_TOK, "after int declaration");
        parse_E();
    }
    void parse_arr_ME()
    { // arr a[k]; E
        expect(IMAS_TOK, "array declaration ('arr')");
        Token id = expect(ID_TOK, "identifier after 'arr'");
        expect(LBRACKET_TOK, "for array size");
        Token size_tok = expect(NUM_TOK, "number for array size");
        int array_size = 0;
        try
        {
            array_size = std::stoi(size_tok.lexeme);
        }
        catch (const std::out_of_range &)
        {
            throwError("Array size number too large: " + size_tok.lexeme);
        }
        catch (const std::invalid_argument &)
        {
            throwError("Invalid number for array size: " + size_tok.lexeme);
        }
        if (array_size <= 0)
            throwError("Array size must be positive for '" + id.lexeme + "'.");
        expect(RBRACKET_TOK, "after array size");
        addSymbol(id.lexeme, SymbolClass::INT_ARRAY, IMAS_TOK, id.line, array_size);
        expect(SEMICOLON_TOK, "after array declaration");
        parse_E();
    }

    // A → aH = G ; A | if ( C ) begin AX ; A | while ( C ) begin A end ; A | cin (aH) ; A | cout ( G ) ; A | λ
    void parse_A()
    {
        Token t = currentToken();
        if (t.code == ID_TOK)
        {
            Token id_token = consumeToken();
            bool is_array_target = false;
            SymbolInfo sym_info = getSymbol(id_token.lexeme, id_token.line);

            if (currentToken().code == LBRACKET_TOK)
            {
                if (sym_info.s_class != SymbolClass::INT_ARRAY)
                    throwError("'" + id_token.lexeme + "' is not an array.");
                is_array_target = true;
                m_rpn.emplace_back(RPNItemType::ARRAY_BASE, id_token.lexeme, id_token.line);
                consumeToken();
                parse_G();
                expect(RBRACKET_TOK, "array index in assignment LHS");
            }
            else
            {
                if (sym_info.s_class == SymbolClass::INT_ARRAY)
                    throwError("Cannot assign to array '" + id_token.lexeme + "' as a whole.");
                m_rpn.emplace_back(RPNItemType::VAR, id_token.lexeme, id_token.line);
            }
            expect(EQ_TOK, "assignment");
            parse_G();
            m_rpn.emplace_back(RPNItemType::OPERATION, (is_array_target ? "[]=" : "="), t.line);
            expect(SEMICOLON_TOK, "after assignment");
            parse_A();
        }
        else if (t.code == IF_TOK)
        {
            consumeToken();
            expect(LPAREN_TOK, "after 'if'");
            parse_C();
            expect(RPAREN_TOK, "after 'if' condition");
            std::string else_label = newLabel(), end_if_label = newLabel();
            m_rpn.emplace_back(RPNItemType::JUMP_FALSE, else_label, t.line);
            expect(BEG_TOK, "'if' block");
            parse_A();

            if (currentToken().code == ELSE_TOK)
            {
                expect(END_TOK, "before 'else'");
                m_rpn.emplace_back(RPNItemType::JUMP, end_if_label, currentToken().line);
                m_rpn.emplace_back(RPNItemType::LABEL_DEF, else_label, currentToken().line);
                consumeToken();
                expect(BEG_TOK, "'else' block");
                parse_A();
                expect(END_TOK, "'else' block");
                m_rpn.emplace_back(RPNItemType::LABEL_DEF, end_if_label, currentToken().line);
            }
            else
            {
                expect(END_TOK, "'if' block (no else)");
                m_rpn.emplace_back(RPNItemType::LABEL_DEF, else_label, t.line); // else_label is where execution continues if condition was false
            }
            expect(SEMICOLON_TOK, "after 'if' statement");
            parse_A();
        }
        else if (t.code == WHILE_TOK)
        {
            consumeToken();
            std::string loop_start = newLabel(), loop_end = newLabel();
            m_rpn.emplace_back(RPNItemType::LABEL_DEF, loop_start, t.line);
            expect(LPAREN_TOK, "after 'while'");
            parse_C();
            expect(RPAREN_TOK, "after 'while' condition");
            m_rpn.emplace_back(RPNItemType::JUMP_FALSE, loop_end, t.line);
            expect(BEG_TOK, "'while' block");
            parse_A();
            expect(END_TOK, "'while' block");
            m_rpn.emplace_back(RPNItemType::JUMP, loop_start, t.line);
            m_rpn.emplace_back(RPNItemType::LABEL_DEF, loop_end, t.line);
            expect(SEMICOLON_TOK, "after 'while' statement");
            parse_A();
        }
        else if (t.code == INPUT_TOK)
        {
            consumeToken();
            expect(LPAREN_TOK, "after 'cin'");
            Token id_token = expect(ID_TOK, "identifier for 'cin'");
            SymbolInfo sym_info = getSymbol(id_token.lexeme, id_token.line);
            if (currentToken().code == LBRACKET_TOK)
            {
                if (sym_info.s_class != SymbolClass::INT_ARRAY)
                    throwError("'" + id_token.lexeme + "' is not an array for cin[].");
                m_rpn.emplace_back(RPNItemType::ARRAY_BASE, id_token.lexeme, id_token.line);
                consumeToken();
                parse_G();
                expect(RBRACKET_TOK, "array index in 'cin'");
                m_rpn.emplace_back(RPNItemType::INPUT, "IN[]", t.line);
            }
            else
            {
                if (sym_info.s_class == SymbolClass::INT_ARRAY)
                    throwError("Cannot 'cin' into array '" + id_token.lexeme + "' as a whole.");
                m_rpn.emplace_back(RPNItemType::VAR, id_token.lexeme, id_token.line);
                m_rpn.emplace_back(RPNItemType::INPUT, "IN", t.line);
            }
            expect(RPAREN_TOK, "after 'cin' target");
            expect(SEMICOLON_TOK, "after 'cin' statement");
            parse_A();
        }
        else if (t.code == OUTPUT_TOK)
        {
            consumeToken();
            expect(LPAREN_TOK, "after 'cout'");
            parse_G();
            expect(RPAREN_TOK, "after 'cout' expression");
            m_rpn.emplace_back(RPNItemType::OUTPUT, "OUT", t.line);
            expect(SEMICOLON_TOK, "after 'cout' statement");
            parse_A();
        }
        else if (t.code == SIN_TOK || t.code == COS_TOK || t.code == TG_TOK || t.code == CTG_TOK)
        {
            // A → sin ( G ) ; A | cos ( G ) ; A | tg ( G ) ; A | ctg ( G ) ; A | ... (остальные правила)
            Token func_token = consumeToken();
            expect(LPAREN_TOK, "after trig function");
            parse_G();
            expect(RPAREN_TOK, "after trig function argument");
            RPNItemType func_type = RPNItemType::FUNC_SIN;
            if (func_token.code == SIN_TOK)
                func_type = RPNItemType::FUNC_SIN;
            else if (func_token.code == COS_TOK)
                func_type = RPNItemType::FUNC_COS;
            else if (func_token.code == TG_TOK)
                func_type = RPNItemType::FUNC_TG;
            else if (func_token.code == CTG_TOK)
                func_type = RPNItemType::FUNC_CTG;
            m_rpn.emplace_back(func_type, func_token.lexeme, func_token.line);
            expect(SEMICOLON_TOK, "after trig function statement");
            parse_A();
        }
        // λ case: if currentToken() is END_TOK or part of an outer structure (like 'else' which means end of 'if' block's A)
        // then this rule effectively produces lambda and returns.
    }

    // G → T U'
    void parse_G()
    {
        parse_T();
        parse_U_prime();
    }
    // U' → + T U' | - T U' | λ
    void parse_U_prime()
    {
        Token t = currentToken();
        if (t.code == PLUS_TOK || t.code == MINUS_TOK)
        {
            consumeToken();
            parse_T();
            m_rpn.emplace_back(RPNItemType::OPERATION, t.lexeme, t.line);
            parse_U_prime();
        }
    }
    // T → F V'
    void parse_T()
    {
        parse_F();
        parse_V_prime();
    }
    // V' → * F V' | / F V' | λ
    void parse_V_prime()
    {
        Token t = currentToken();
        if (t.code == STAR_TOK || t.code == SLASH_TOK)
        {
            consumeToken();
            parse_F();
            m_rpn.emplace_back(RPNItemType::OPERATION, t.lexeme, t.line);
            parse_V_prime();
        }
    }
    // F → (G) | aH | k | sin(G) | cos(G) | tg(G) | ctg(G)
    void parse_F()
    {
        Token t = currentToken();
        if (t.code == LPAREN_TOK)
        {
            consumeToken();
            parse_G();
            expect(RPAREN_TOK, "closing ')' in expression");
        }
        else if (t.code == SIN_TOK || t.code == COS_TOK || t.code == TG_TOK || t.code == CTG_TOK)
        {
            Token func_token = consumeToken();
            expect(LPAREN_TOK, "after trig function");
            parse_G();
            expect(RPAREN_TOK, "after trig function argument");
            RPNItemType func_type = RPNItemType::FUNC_SIN;
            if (func_token.code == SIN_TOK)
                func_type = RPNItemType::FUNC_SIN;
            else if (func_token.code == COS_TOK)
                func_type = RPNItemType::FUNC_COS;
            else if (func_token.code == TG_TOK)
                func_type = RPNItemType::FUNC_TG;
            else if (func_token.code == CTG_TOK)
                func_type = RPNItemType::FUNC_CTG;
            m_rpn.emplace_back(func_type, func_token.lexeme, func_token.line);
        }
        else if (t.code == ID_TOK)
        {
            Token id_token = consumeToken();
            SymbolInfo sym_info = getSymbol(id_token.lexeme, id_token.line);
            if (currentToken().code == LBRACKET_TOK)
            {
                if (sym_info.s_class != SymbolClass::INT_ARRAY)
                    throwError("'" + id_token.lexeme + "' is not an array for indexing.");
                m_rpn.emplace_back(RPNItemType::ARRAY_BASE, id_token.lexeme, id_token.line);
                consumeToken();
                parse_G();
                expect(RBRACKET_TOK, "array index in expression");
                m_rpn.emplace_back(RPNItemType::ARRAY_ACCESS, "[]", t.line);
            }
            else
            {
                if (sym_info.s_class == SymbolClass::INT_ARRAY)
                    throwError("Cannot use array '" + id_token.lexeme + "' as simple value.");
                m_rpn.emplace_back(RPNItemType::VAR, id_token.lexeme, id_token.line);
            }
        }
        else if (t.code == NUM_TOK)
        {
            consumeToken();
            m_rpn.emplace_back(RPNItemType::CONST, t.lexeme, t.line);
        }
        else
        {
            throwError("Invalid start of factor: expected '(', identifier, or number, got " + t.codeToString());
        }
    }

    // C → G D G (D → ~ | > | < | !)
    void parse_C()
    {
        parse_G();
        Token op_tok = currentToken();
        std::string op_str;
        if (op_tok.code == EQ_COMPARE_TOK)
        {
            op_str = "~";
            consumeToken();
        }
        else if (op_tok.code == GT_TOK)
        {
            op_str = ">";
            consumeToken();
        }
        else if (op_tok.code == LT_TOK)
        {
            op_str = "<";
            consumeToken();
        }
        else if (op_tok.code == NOT_TOK)
        {
            op_str = "!";
            consumeToken();
        }
        else
        {
            throwError("Expected relational operator (~, >, <, !) in condition, found " + op_tok.codeToString());
        }
        parse_G();
        m_rpn.emplace_back(RPNItemType::OPERATION, op_str, op_tok.line);
    }
};

// RPN Interpreter Class
class RPNInterpreter
{
public:
    RPNInterpreter(const std::vector<RPNEntry> &rpn, const std::map<std::string, SymbolInfo> &symbolTable)
        : m_rpn(rpn), m_symbolTable(symbolTable), m_pc(0)
    {
        // Pre-populate variables and arrays from symbol table
        for (const auto &sym_pair : m_symbolTable)
        {
            const std::string &name = sym_pair.first;
            const SymbolInfo &info = sym_pair.second;
            if (info.is_declared)
            { // Ensure it was actually declared
                if (info.s_class == SymbolClass::INT_VAR)
                {
                    m_variables[name] = 0; // Default initialize to 0
                }
                else if (info.s_class == SymbolClass::INT_ARRAY)
                {
                    m_arrays[name] = std::vector<int>(info.size, 0); // Default initialize elements to 0
                }
            }
        }
        // Build label map
        for (size_t i = 0; i < m_rpn.size(); ++i)
        {
            if (m_rpn[i].type == RPNItemType::LABEL_DEF)
            {
                if (m_labelMap.count(m_rpn[i].value))
                {
                    throw std::runtime_error("Interpreter Setup Error: Duplicate label definition '" + m_rpn[i].value + "'. This should be caught by parser.");
                }
                m_labelMap[m_rpn[i].value] = i;
            }
        }
    }

    void run()
    {
        m_pc = 0;
        m_operandStack.clear();

        while (m_pc < m_rpn.size())
        {
            const RPNEntry &entry = m_rpn[m_pc];
            // For debugging:
            // std::cout << "Executing PC " << m_pc << ": " << entry.typeToString() << " \"" << entry.value
            //           << "\" (Line: " << entry.line_num << ")" << std::endl;

            bool increment_pc = true;

            try
            {
                switch (entry.type)
                {
                case RPNItemType::VAR:
                    // VAR RPN item means "push variable NAME" onto operand stack.
                    // Subsequent operations (like arithmetic or assignment) will resolve this name to a value or use it as a target.
                    m_operandStack.push_back(entry.value);
                    break;

                case RPNItemType::ARRAY_BASE:
                    // ARRAY_BASE RPN item means "push array NAME" onto operand stack.
                    m_operandStack.push_back(entry.value);
                    break;

                case RPNItemType::CONST:
                    try
                    {
                        m_operandStack.push_back(std::stoi(entry.value));
                    }
                    catch (const std::out_of_range &)
                    {
                        throw std::runtime_error("Invalid constant (too large/small): '" + entry.value + "'");
                    }
                    catch (const std::invalid_argument &)
                    {
                        throw std::runtime_error("Invalid constant (not a number): '" + entry.value + "'");
                    }
                    break;

                case RPNItemType::OPERATION:
                    handle_operation(entry);
                    break;

                case RPNItemType::LABEL_DEF:
                    // NOP during execution, handled in constructor
                    break;

                case RPNItemType::JUMP:
                    m_pc = find_label(entry.value, entry.line_num);
                    increment_pc = false; // PC is set directly, don't increment at the end
                    break;

                case RPNItemType::JUMP_FALSE:
                {
                    StackItem condition_item = pop_operand();
                    int condition = get_int(condition_item, "Condition for JUMP_FALSE");
                    if (condition == 0)
                    { // If condition is false (0)
                        m_pc = find_label(entry.value, entry.line_num);
                        increment_pc = false;
                    }
                    break;
                }

                case RPNItemType::ARRAY_ACCESS: // "[]" operation
                    handle_array_access(entry);
                    break;

                    // RPNItemType::ARRAY_ASSIGN is not used; "[]=" is an OPERATION.

                case RPNItemType::INPUT:
                    handle_input(entry);
                    break;

                case RPNItemType::OUTPUT:
                    handle_output(entry);
                    break;

                case RPNItemType::FUNC_SIN:
                {
                    StackItem arg = pop_operand();
                    int val = get_int(arg, "sin argument");
                    push_operand(static_cast<int>(std::sin(val)));
                    break;
                }
                case RPNItemType::FUNC_COS:
                {
                    StackItem arg = pop_operand();
                    int val = get_int(arg, "cos argument");
                    push_operand(static_cast<int>(std::cos(val)));
                    break;
                }
                case RPNItemType::FUNC_TG:
                {
                    StackItem arg = pop_operand();
                    int val = get_int(arg, "tg argument");
                    push_operand(static_cast<int>(std::tan(val)));
                    break;
                }
                case RPNItemType::FUNC_CTG:
                {
                    StackItem arg = pop_operand();
                    int val = get_int(arg, "ctg argument");
                    if (std::tan(val) == 0)
                        throw std::runtime_error("ctg undefined (tan(x)==0)");
                    push_operand(static_cast<int>(1.0 / std::tan(val)));
                    break;
                }

                default:
                    throw std::runtime_error("Unknown RPN item type: " + entry.typeToString());
                }
            }
            catch (const std::runtime_error &e)
            {
                // Augment error with line number from RPN entry
                throw std::runtime_error("Interpreter Error (Source Line " + std::to_string(entry.line_num) +
                                         ", RPN PC " + std::to_string(m_pc) + "): " + e.what());
            }

            if (increment_pc)
            {
                m_pc++;
            }
            // Debug operand stack:
            // print_operand_stack_debug();
        }
    }

private:
    using StackItem = std::variant<int, std::string>;
    std::vector<StackItem> m_operandStack;
    std::map<std::string, int> m_variables;
    std::map<std::string, std::vector<int>> m_arrays;
    std::map<std::string, size_t> m_labelMap;

    const std::vector<RPNEntry> &m_rpn;
    const std::map<std::string, SymbolInfo> &m_symbolTable;
    size_t m_pc;

    void push_operand(StackItem val) { m_operandStack.push_back(val); }
    StackItem pop_operand()
    {
        if (m_operandStack.empty())
        {
            throw std::runtime_error("Operand stack underflow.");
        }
        StackItem val = m_operandStack.back();
        m_operandStack.pop_back();
        return val;
    }

    int get_int(const StackItem &item, const std::string &context)
    {
        if (std::holds_alternative<int>(item))
        {
            return std::get<int>(item);
        }
        else if (std::holds_alternative<std::string>(item))
        {
            const std::string &var_name = std::get<std::string>(item);
            auto it_var = m_variables.find(var_name);
            if (it_var != m_variables.end())
            {
                return it_var->second;
            }
            // It's not a simple variable, check if it's an array name (which shouldn't be directly converted to int here)
            // This situation typically means an array name was used where a value was expected without indexing.
            // The parser should catch most of these, but a runtime check is good.
            if (m_arrays.count(var_name))
            {
                throw std::runtime_error("Cannot use array '" + var_name + "' as a simple integer value for " + context +
                                         ". Array must be indexed.");
            }
            throw std::runtime_error("Undeclared identifier or uninitialized variable '" + var_name + "' used as integer for " + context + ".");
        }
        throw std::runtime_error("Invalid type on operand stack for " + context + ". Expected int or resolvable variable name.");
    }

    std::string get_string(const StackItem &item, const std::string &context)
    {
        if (std::holds_alternative<std::string>(item))
        {
            return std::get<std::string>(item);
        }
        // If an int is found where a string (name) was expected.
        int int_val = 0;
        if (std::holds_alternative<int>(item))
            int_val = std::get<int>(item);
        throw std::runtime_error("Invalid type on operand stack for " + context + ". Expected string (identifier name), but found " +
                                 (std::holds_alternative<int>(item) ? "integer " + std::to_string(int_val) : "unknown type") + ".");
    }

    void handle_operation(const RPNEntry &entry)
    {
        const std::string &op = entry.value;

        if (op == "=")
        {
            StackItem rhs_item = pop_operand();
            StackItem lhs_item = pop_operand();

            int val_to_assign = get_int(rhs_item, "RHS of assignment");
            std::string var_name = get_string(lhs_item, "LHS of assignment (variable name)");

            if (m_variables.find(var_name) == m_variables.end())
            {
                // Check if it's an array name - cannot assign to entire array this way
                if (m_arrays.count(var_name))
                {
                    throw std::runtime_error("Cannot assign to array '" + var_name + "' as a whole. Use indexed assignment.");
                }
                throw std::runtime_error("Assignment to undeclared variable '" + var_name + "'.");
            }
            m_variables[var_name] = val_to_assign;
        }
        else if (op == "[]=")
        {
            StackItem val_item = pop_operand();
            StackItem idx_item = pop_operand();
            StackItem arr_name_item = pop_operand();

            int value_to_assign = get_int(val_item, "Value for array assignment");
            int index = get_int(idx_item, "Index for array assignment");
            std::string array_name = get_string(arr_name_item, "Array name for assignment");

            auto it = m_arrays.find(array_name);
            if (it == m_arrays.end())
            {
                throw std::runtime_error("Assignment to undeclared array '" + array_name + "'.");
            }
            if (index < 0 || static_cast<size_t>(index) >= it->second.size())
            {
                throw std::runtime_error("Array index " + std::to_string(index) + " out of bounds for array '" + array_name +
                                         "' (size " + std::to_string(it->second.size()) + ").");
            }
            it->second[index] = value_to_assign;
        }
        else
        {
            StackItem rhs_item = pop_operand();
            StackItem lhs_item = pop_operand();

            int b = get_int(rhs_item, "RHS of operation '" + op + "'");
            int a = get_int(lhs_item, "LHS of operation '" + op + "'");
            int result = 0;

            if (op == "+")
                result = a + b;
            else if (op == "-")
                result = a - b;
            else if (op == "*")
                result = a * b;
            else if (op == "/")
            {
                if (b == 0)
                    throw std::runtime_error("Division by zero.");
                result = a / b;
            }
            else if (op == "~")
                result = (a == b ? 1 : 0); // Return 1 for true, 0 for false
            else if (op == ">")
                result = (a > b ? 1 : 0);
            else if (op == "<")
                result = (a < b ? 1 : 0);
            else if (op == "!")
                result = (a != b ? 1 : 0); // Not equal
            else
            {
                throw std::runtime_error("Unknown arithmetic/logical operator '" + op + "'.");
            }
            push_operand(result);
        }
    }

    void handle_array_access(const RPNEntry &entry)
    {
        StackItem idx_item = pop_operand();
        StackItem arr_name_item = pop_operand();

        int index = get_int(idx_item, "Index for array access");
        std::string array_name = get_string(arr_name_item, "Array name for access");

        auto it = m_arrays.find(array_name);
        if (it == m_arrays.end())
        {
            throw std::runtime_error("Access to undeclared array '" + array_name + "'.");
        }
        if (index < 0 || static_cast<size_t>(index) >= it->second.size())
        {
            throw std::runtime_error("Array index " + std::to_string(index) + " out of bounds for array '" + array_name +
                                     "' (size " + std::to_string(it->second.size()) + ").");
        }
        push_operand(it->second[index]);
    }

    void handle_input(const RPNEntry &entry)
    {
        const std::string &input_type = entry.value; // "IN" or "IN[]"
        int val;
        std::cout << "Input (integer): ";
        if (!(std::cin >> val))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            throw std::runtime_error("Invalid input, integer expected.");
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (input_type == "IN")
        {
            StackItem var_name_item = pop_operand();
            std::string var_name = get_string(var_name_item, "Target variable for input");
            if (m_variables.find(var_name) == m_variables.end())
            {
                if (m_arrays.count(var_name))
                { // Check if it's an array name
                    throw std::runtime_error("Cannot 'cin' into array '" + var_name + "' as a whole. Use indexed input.");
                }
                throw std::runtime_error("Input to undeclared variable '" + var_name + "'.");
            }
            m_variables[var_name] = val;
        }
        else if (input_type == "IN[]")
        {
            StackItem idx_item = pop_operand();
            StackItem arr_name_item = pop_operand();

            int index = get_int(idx_item, "Index for array input");
            std::string array_name = get_string(arr_name_item, "Array name for input");

            auto it = m_arrays.find(array_name);
            if (it == m_arrays.end())
            {
                throw std::runtime_error("Input to undeclared array '" + array_name + "'.");
            }
            if (index < 0 || static_cast<size_t>(index) >= it->second.size())
            {
                throw std::runtime_error("Array index " + std::to_string(index) + " out of bounds for input to array '" + array_name +
                                         "' (size " + std::to_string(it->second.size()) + ").");
            }
            it->second[index] = val;
        }
        else
        {
            throw std::runtime_error("Unknown input type '" + input_type + "'.");
        }
    }

    void handle_output(const RPNEntry &entry)
    {
        StackItem val_item = pop_operand();
        int val_to_print = get_int(val_item, "Value for output");
        std::cout << "Output: " << val_to_print << std::endl;
    }

    size_t find_label(const std::string &label_name, int source_line_for_error)
    {
        auto it = m_labelMap.find(label_name);
        if (it == m_labelMap.end())
        {
            throw std::runtime_error("Undefined label '" + label_name + "' targeted by jump from source line " + std::to_string(source_line_for_error) + ".");
        }
        return it->second;
    }

    void print_operand_stack_debug()
    {
        std::cout << "  Interpreter Operand Stack (PC " << m_pc << "): [";
        for (size_t i = 0; i < m_operandStack.size(); ++i)
        {
            if (std::holds_alternative<int>(m_operandStack[i]))
            {
                std::cout << std::get<int>(m_operandStack[i]);
            }
            else
            {
                std::cout << "\"" << std::get<std::string>(m_operandStack[i]) << "\"";
            }
            if (i < m_operandStack.size() - 1)
                std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
};

// --- Вспомогательные функции для main ---
std::string symbolTypeToString(TokenCode tc)
{
    Token temp(tc, "");
    return temp.codeToString();
}
std::string symbolClassToString(SymbolClass sc)
{
    switch (sc)
    {
    case SymbolClass::INT_VAR:
        return "INT_VARIABLE";
    case SymbolClass::INT_ARRAY:
        return "INT_ARRAY";
    case SymbolClass::UNKNOWN:
        return "UNKNOWN_CLASS";
    default:
        return "INVALID_CLASS_CODE(" + std::to_string(static_cast<int>(sc)) + ")";
    }
}

// Main Function
int main()
{
    std::cout << "Введите путь к файлу с кодом или введите код вручную (завершите EOF - Ctrl+D/Ctrl+Z+Enter):\n";
    std::string filepath_or_code;
    std::cout << "Путь к файлу (или 'manual' для ручного ввода): ";
    std::getline(std::cin, filepath_or_code);

    std::istream *inputStreamPtr = nullptr;
    std::ifstream fileStream;
    std::stringstream stringStream;

    if (filepath_or_code == "manual")
    {
        std::cout << "Введите ваш код. Завершите EOF (Ctrl+D в Linux/macOS, Ctrl+Z затем Enter в Windows).\n";
        std::cout << "-------------------------------------------------------\n";
        std::string line;
        std::string full_code;
        while (std::getline(std::cin, line))
        {
            full_code += line + "\n";
        }
        stringStream.str(full_code);
        inputStreamPtr = &stringStream;
    }
    else
    {
        fileStream.open(filepath_or_code);
        if (!fileStream.is_open())
        {
            std::cerr << "Не удалось открыть файл: " << filepath_or_code << std::endl;
            return 1;
        }
        inputStreamPtr = &fileStream;
        std::cout << "Чтение из файла: " << filepath_or_code << std::endl;
    }

    Lexer lexer(*inputStreamPtr);
    std::vector<Token> tokens;
    Token t;

    std::cout << "\n--- Распознанные токены ---" << std::endl;
    do
    {
        t = lexer.getNextToken();
        if (t.code != NONE_TOK)
        { // Avoid printing NONE_TOK if lexer has internal empty states
            std::cout << "  " << t.codeToString() << " : \"" << t.lexeme << "\" (Line: " << t.line << ")" << std::endl;
            tokens.push_back(t);
        }
        if (t.code == ERROR_TOK)
        {
            std::cerr << "Лексический анализ остановлен из-за ошибки." << std::endl;
            return 1;
        }
        if (tokens.size() > 10000)
        { // Increased limit slightly
            std::cerr << "Слишком много токенов (>10000), прерывание." << std::endl;
            return 1;
        }
    } while (t.code != EOF_TOK);
    std::cout << "--- Конец списка токенов ---\n"
              << std::endl;

    if (tokens.empty() || (tokens.size() == 1 && tokens.back().code == EOF_TOK))
    {
        std::cout << "Нет токенов для парсинга (кроме EOF)." << std::endl;
        return 0;
    }

    try
    {
        RPNGenerator rpnGen(tokens);
        std::vector<RPNEntry> rpn_output = rpnGen.generate();

        std::cout << "--- ОПЗ (RPN) ---" << std::endl;
        if (rpn_output.empty())
            std::cout << "  (пусто)" << std::endl;
        int rpn_idx = 0;
        for (const auto &entry : rpn_output)
        {
            std::cout << "  " << rpn_idx++ << ": Line " << entry.line_num << ": " << entry.typeToString()
                      << " Value: \"" << entry.value << "\"" << std::endl;
        }
        std::cout << "--- Конец ОПЗ ---\n"
                  << std::endl;

        std::cout << "--- Таблица символов ---" << std::endl;
        if (rpnGen.getSymbolTable().empty())
            std::cout << "  (пусто)" << std::endl;
        for (const auto &pair : rpnGen.getSymbolTable())
        {
            std::cout << "  '" << pair.first << "':"
                      << " Class=" << symbolClassToString(pair.second.s_class)
                      << ", TypeToken=" << symbolTypeToString(pair.second.type_token)
                      << ", Size=" << pair.second.size
                      << ", DeclLine=" << pair.second.declaration_line
                      << ", Declared=" << (pair.second.is_declared ? "true" : "false") << std::endl;
        }
        std::cout << "--- Конец таблицы символов ---\n"
                  << std::endl;

        std::cout << "--- Запуск интерпретатора ОПЗ ---" << std::endl;
        RPNInterpreter interpreter(rpn_output, rpnGen.getSymbolTable());
        interpreter.run();
        std::cout << "--- Интерпретация завершена ---" << std::endl;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}