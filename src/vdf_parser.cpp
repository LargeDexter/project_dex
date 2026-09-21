#include "vdf_parser.h"

namespace {

struct Token {
    enum Type { String, OpenBrace, CloseBrace, End } type;
    QString text;
};

class Tokenizer
{
public:
    explicit Tokenizer(const QString &input) : m_input(input) {}

    Token next()
    {
        skipWhitespaceAndComments();
        if (m_pos >= m_input.size())
            return {Token::End, {}};

        const QChar c = m_input.at(m_pos);
        if (c == '{') { ++m_pos; return {Token::OpenBrace, {}}; }
        if (c == '}') { ++m_pos; return {Token::CloseBrace, {}}; }

        if (c == '"') {
            ++m_pos;
            QString value;
            while (m_pos < m_input.size() && m_input.at(m_pos) != '"') {
                if (m_input.at(m_pos) == '\\' && m_pos + 1 < m_input.size()) {
                    value += m_input.at(m_pos + 1);
                    m_pos += 2;
                } else {
                    value += m_input.at(m_pos);
                    ++m_pos;
                }
            }
            if (m_pos < m_input.size())
                ++m_pos; // closing quote
            return {Token::String, value};
        }

        // Unquoted token -- rare in Steam's own files, but tolerate it.
        QString value;
        while (m_pos < m_input.size() && !m_input.at(m_pos).isSpace()
               && m_input.at(m_pos) != '{' && m_input.at(m_pos) != '}') {
            value += m_input.at(m_pos);
            ++m_pos;
        }
        return {Token::String, value};
    }

private:
    void skipWhitespaceAndComments()
    {
        while (m_pos < m_input.size()) {
            const QChar c = m_input.at(m_pos);
            if (c.isSpace()) { ++m_pos; continue; }
            if (c == '/' && m_pos + 1 < m_input.size() && m_input.at(m_pos + 1) == '/') {
                while (m_pos < m_input.size() && m_input.at(m_pos) != '\n')
                    ++m_pos;
                continue;
            }
            break;
        }
    }

    QString m_input;
    int m_pos = 0;
};

VdfNode parseBlock(Tokenizer &tok)
{
    VdfNode node;
    node.isBlock = true;

    while (true) {
        Token keyTok = tok.next();
        if (keyTok.type == Token::CloseBrace || keyTok.type == Token::End)
            break;
        if (keyTok.type != Token::String)
            continue;

        Token valueTok = tok.next();
        if (valueTok.type == Token::OpenBrace) {
            node.children.insert(keyTok.text, parseBlock(tok));
        } else if (valueTok.type == Token::String) {
            VdfNode leaf;
            leaf.value = valueTok.text;
            node.children.insert(keyTok.text, leaf);
        } else {
            break; // malformed -- stop rather than loop forever
        }
    }

    return node;
}

} // namespace

VdfNode parseVdf(const QString &text)
{
    Tokenizer tok(text);

    Token first = tok.next();
    if (first.type != Token::String)
        return {};

    Token brace = tok.next();
    if (brace.type != Token::OpenBrace)
        return {};

    VdfNode root;
    root.isBlock = true;
    root.children.insert(first.text, parseBlock(tok));
    return root;
}
