using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

public static class CodeStyleParagraphSpacing
{
    // Mask lexical contents without changing positions or line boundaries.
    private static readonly Regex LexicalContents = new Regex(
        "(?m)^[ \\t]*//[ \\t]*clang-format off\\b[\\s\\S]*?(?:^[ \\t]*//[ \\t]*clang-format on[^\\n]*|\\z)|" +
        "^[ \\t]*#(?:[^\\n]*\\\\\\n)*[^\\n]*|" +
        "(?:u8|u|U|L)?R\"(?<delimiter>[^ ()\\\\\\t\\r\\n]{0,16})\\([\\s\\S]*?\\)\\k<delimiter>\"|" +
        "(?:u8|u|U|L)?\"(?:\\\\[\\s\\S]|[^\"\\\\])*\"|" +
        "(?<![\\w])(?:u8|u|U|L)?'(?:\\\\[^\\n]|[^'\\\\\\n])*'|//[^\\n]*|/\\*[\\s\\S]*?\\*/",
        RegexOptions.Compiled);

    private static readonly Regex CallHead = new Regex(
        @"^(?:(?:[A-Za-z_]\w*)(?:\s*::\s*|\s*\.\s*|\s*->\s*))*[A-Za-z_]\w*\s*(?:<[^;{}]*>)?\s*\(",
        RegexOptions.Compiled);

    private static readonly Regex ControlHead = new Regex(
        @"^(?:if|else|for|while|switch|catch|do|class|struct|enum|namespace|extern|using|typedef)\b",
        RegexOptions.Compiled);

    public static string Format(string text)
    {
        text = text.Replace("\r\n", "\n").Replace("\r", "\n");
        string[] lines = text.Split('\n');
        int[] lineStarts = GetLineStarts(text);
        Dictionary<int, int> leadingComments = new Dictionary<int, int>();
        string masked = MaskContents(text, lineStarts, leadingComments);
        HashSet<int> boundaries = FindParagraphBoundaries(masked, lines, lineStarts, leadingComments);

        if (boundaries.Count == 0)
        {
            return text;
        }

        StringBuilder result = new StringBuilder(text.Length + boundaries.Count);
        for (int line = 0; line < lines.Length; ++line)
        {
            if (boundaries.Contains(line))
            {
                result.Append('\n');
            }

            result.Append(lines[line]);
            if (line + 1 < lines.Length)
            {
                result.Append('\n');
            }
        }

        return result.ToString();
    }

    private static string MaskContents(string text, int[] lineStarts, Dictionary<int, int> leadingComments)
    {
        return LexicalContents.Replace(text, delegate(Match match)
        {
            if (match.Value.StartsWith("//") || match.Value.StartsWith("/*"))
            {
                int firstLine = GetLine(lineStarts, match.Index);
                int lastLine = GetLine(lineStarts, match.Index + match.Length - 1);
                int lineEnd = lastLine + 1 < lineStarts.Length ? lineStarts[lastLine + 1] : text.Length;
                string prefix = text.Substring(lineStarts[firstLine], match.Index - lineStarts[firstLine]);
                string suffix = text.Substring(match.Index + match.Length, lineEnd - match.Index - match.Length);
                if (string.IsNullOrWhiteSpace(prefix) && string.IsNullOrWhiteSpace(suffix))
                {
                    leadingComments[lastLine] = firstLine;
                }
            }

            char[] characters = match.Value.ToCharArray();
            for (int index = 0; index < characters.Length; ++index)
            {
                if (characters[index] != '\n')
                {
                    characters[index] = ' ';
                }
            }

            return new string(characters);
        });
    }

    private static int[] GetLineStarts(string text)
    {
        List<int> starts = new List<int>();
        starts.Add(0);
        for (int index = 0; index < text.Length; ++index)
        {
            if (text[index] == '\n')
            {
                starts.Add(index + 1);
            }
        }

        return starts.ToArray();
    }

    private static int GetLine(int[] starts, int position)
    {
        int line = Array.BinarySearch(starts, position);
        return line >= 0 ? line : ~line - 1;
    }

    private static HashSet<int> FindParagraphBoundaries(string masked, string[] lines, int[] lineStarts, Dictionary<int, int> leadingComments)
    {
        HashSet<int> boundaries = new HashSet<int>();
        int start = -1;
        int parentheses = 0;
        int brackets = 0;
        int expressionBraces = 0;
        Stack<bool> blocks = new Stack<bool>();

        for (int position = 0; position < masked.Length; ++position)
        {
            char character = masked[position];
            if (char.IsWhiteSpace(character))
            {
                continue;
            }

            if (start < 0)
            {
                start = position;
            }

            switch (character)
            {
                case '(':
                    ++parentheses;
                    break;
                case ')':
                    parentheses = Math.Max(0, parentheses - 1);
                    break;
                case '[':
                    ++brackets;
                    break;
                case ']':
                    brackets = Math.Max(0, brackets - 1);
                    break;
                case '{':
                    if (expressionBraces > 0 || parentheses > 0 || brackets > 0 || IsInitializer(masked.Substring(start, position - start)))
                    {
                        ++expressionBraces;
                    }
                    else
                    {
                        string head = masked.Substring(start, position - start).Trim();
                        bool isTypeOrNamespace = Regex.IsMatch(head, @"^(?:class|struct|enum|namespace|extern)\b");
                        bool executable = !isTypeOrNamespace && (head.Contains(")") || (blocks.Count > 0 && blocks.Peek()));
                        blocks.Push(executable);
                        start = -1;
                    }
                    break;
                case '}':
                    if (expressionBraces > 0)
                    {
                        --expressionBraces;
                    }
                    else
                    {
                        if (blocks.Count > 0)
                        {
                            blocks.Pop();
                        }

                        start = -1;
                    }
                    break;
                case ';':
                    if (parentheses == 0 && brackets == 0 && expressionBraces == 0)
                    {
                        AddStatementBoundaries(masked, start, position, lines, lineStarts, boundaries, leadingComments, blocks.Count > 0 && blocks.Peek());
                        start = -1;
                    }
                    break;
            }
        }

        return boundaries;
    }

    private static bool HasAssignment(string text)
    {
        int parentheses = 0;
        int brackets = 0;
        for (int index = 0; index < text.Length; ++index)
        {
            char character = text[index];
            if (character == '(')
            {
                ++parentheses;
            }
            if (character == ')')
            {
                --parentheses;
            }
            if (character == '[')
            {
                ++brackets;
            }
            if (character == ']')
            {
                --brackets;
            }
            if (character != '=' || parentheses != 0 || brackets != 0)
            {
                continue;
            }

            char previous = index > 0 ? text[index - 1] : '\0';
            char next = index + 1 < text.Length ? text[index + 1] : '\0';
            if (next != '=' && previous != '=' && previous != '!' && previous != '<' && previous != '>')
            {
                return true;
            }
        }

        return false;
    }

    private static bool IsInitializer(string head)
    {
        head = head.Trim();
        if (ControlHead.IsMatch(head))
        {
            return false;
        }

        return HasAssignment(head) || Regex.IsMatch(head, @"^(?:return|co_return|throw)\b") ||
            (head.IndexOf('(') < 0 && Regex.IsMatch(head, @"\s+[A-Za-z_]\w*\s*(?:\[[^\]]*\])?$"));
    }

    private static bool IsStatement(string statement, bool executableBlock)
    {
        if (ControlHead.IsMatch(statement))
        {
            return false;
        }

        int brace = statement.IndexOf('{');
        return (executableBlock && CallHead.IsMatch(statement)) || HasAssignment(statement) || Regex.IsMatch(statement, @"^(?:return|co_return|throw)\b") ||
            (brace >= 0 && IsInitializer(statement.Substring(0, brace)));
    }

    private static bool SuppliesBeforeBoundary(string line)
    {
        line = line.Trim();
        return line.Length == 0 || line == "{" || line.EndsWith("{") || line.StartsWith("#") ||
            Regex.IsMatch(line, @"^(?:public|protected|private):$");
    }

    private static bool SuppliesAfterBoundary(string line)
    {
        line = line.Trim();
        return line.Length == 0 || line.StartsWith("}") || line.StartsWith("#");
    }

    private static void AddStatementBoundaries(string masked, int start, int end, string[] lines, int[] lineStarts, HashSet<int> boundaries, Dictionary<int, int> leadingComments, bool executableBlock)
    {
        int firstLine = GetLine(lineStarts, start);
        int lastLine = GetLine(lineStarts, end);
        if (firstLine == lastLine || !IsStatement(masked.Substring(start, end - start + 1).Trim(), executableBlock))
        {
            return;
        }

        int commentStart;
        while (leadingComments.TryGetValue(firstLine - 1, out commentStart))
        {
            firstLine = commentStart;
        }

        if (firstLine > 0 && !SuppliesBeforeBoundary(lines[firstLine - 1]))
        {
            boundaries.Add(firstLine);
        }

        if (lastLine + 1 < lines.Length && !SuppliesAfterBoundary(lines[lastLine + 1]))
        {
            boundaries.Add(lastLine + 1);
        }
    }
}
