#include "text_format.hpp"

#include <algorithm>

#include "init.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

// Reads and removes the first word of the string.
static std::string read_and_remove_word(std::string& line)
{
        std::string str = "";

        for (auto it = begin(line); it != end(line); /* No increment */)
        {
                const char current_char = *it;

                line.erase(it);

                if (current_char == ' ')
                {
                        break;
                }

                str += current_char;
        }

        return str;
}

static bool is_word_fit(
        const std::string& current_string,
        const std::string& word_to_fit,
        const size_t max_w)
{
        return (current_string.size() + word_to_fit.size() + 1) <= max_w;
}

// -----------------------------------------------------------------------------
// text_format
// -----------------------------------------------------------------------------
namespace text_format
{

std::vector<std::string> split(std::string line, const int max_w)
{
        if (line.empty())
        {
                return {};
        }

        std::string current_word = read_and_remove_word(line);

        if (line.empty())
        {
                return {current_word};
        }

        std::vector<std::string> result = {""};

        size_t current_row_idx = 0;

        while (!current_word.empty())
        {
                if (!is_word_fit(result[current_row_idx], current_word, max_w))
                {
                        // Word did not fit on current line, make a new line
                        ++current_row_idx;

                        result.push_back("");
                }

                // If this is not the first word on the current line, add a
                // space before the word
                if (!result[current_row_idx].empty())
                {
                        result[current_row_idx] += " ";
                }

                result[current_row_idx] += current_word;

                current_word = read_and_remove_word(line);
        }

        return result;
}

std::vector<std::string> space_separated_list(const std::string& line)
{
        std::vector<std::string> result;

        std::string current_line = "";

        for (char c : line)
        {
                if (c == ' ')
                {
                        result.push_back(current_line);

                        current_line = "";
                }
                else
                {
                        current_line += c;
                }
        }

        return result;
}

std::string replace_all(
        const std::string& line,
        const std::string& from,
        const std::string& to)
{
        std::string result;

        if (from.empty())
        {
                return result;
        }

        result = line;

        size_t start_pos = 0;

        while ((start_pos = result.find(from, start_pos)) != std::string::npos)
        {
                result.replace(start_pos, from.length(), to);

                //In case 'to' contains 'from', like replacing 'x' with 'yx'
                start_pos += to.length();
        }

        return result;
}

std::string pad_before(
        const std::string& str,
        const size_t tot_w,
        const char c)
{
        std::string result = str;

        if (tot_w > str.size())
        {
                result.insert(0, tot_w - result.size(), c);
        }

        return result;
}

std::string pad_after(
        const std::string& str,
        const size_t tot_w,
        const char c)
{
        std::string result = str;

        if (tot_w > result.size())
        {
                result.insert(result.size(), tot_w, c);
        }

        return result;
}

std::string first_to_lower(const std::string& str)
{
        std::string result = str;

        if (!result.empty())
        {
                result[0] = tolower(result[0]);
        }

        return result;
}

std::string first_to_upper(const std::string& str)
{
        std::string result = str;

        if (!result.empty())
        {
                result[0] = toupper(result[0]);
        }

        return result;
}

std::string all_to_upper(const std::string& str)
{
        std::string result = str;

        transform(begin(result), end(result), begin(result), ::toupper);

        return result;
}

void append_with_space(
        std::string& base_str,
        const std::string& addition)
{
        if (!base_str.empty() && !addition.empty())
        {
                base_str += " ";
        }

        base_str += addition;
}

} // text_format
