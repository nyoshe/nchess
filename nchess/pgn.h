#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include "pgn.h"
#include <fstream>

namespace pgn
{
    struct PgnMove
    {
        std::string san;
        int ply;
        int eval;
        bool isCheck;
        bool isCheckmate;
        bool isStalemate;
    };



    inline std::vector<PgnMove> parse_pgn_moves(const std::string& pgnText)
    {
        std::vector<PgnMove> moves;
        std::istringstream iss(pgnText);
        std::string token;
        int ply = 1;
        int lastEval = 0; // Default eval if not present

        // PGN results to skip
        const std::vector<std::string> results = { "1-0", "0-1", "1/2-1/2", "*" };

        while (iss >> token)
        {
            // Handle eval comments: { [%eval 0.23] }
            if (token == "{")
            {
                std::string comment, word;
                // Read until closing '}'
                while (iss >> word && word != "}")
                    comment += word + " ";
                comment = comment.substr(0, comment.size() - 1); // Remove trailing space

                // Look for [%eval ...]
                auto evalPos = comment.find("[%eval");
                if (evalPos != std::string::npos)
                {
                    std::string evalStr = comment.substr(evalPos + 6);
                    // Remove leading/trailing spaces and brackets
                    evalStr.erase(0, evalStr.find_first_not_of(" ["));
                    evalStr.erase(evalStr.find_last_not_of(" ]") + 1);

                    // Now evalStr should be like "0.23" or "#5"
                    if (!moves.empty())
                    {
                        if (!evalStr.empty() && evalStr[0] == '#')
                        {
                            // Mate score
                            try {
                                moves.back().eval = (evalStr[1] == '-') ? -100000 + std::stoi(evalStr.substr(2)) : 100000 - std::stoi(evalStr.substr(1));
                            }
                            catch (...) {
                                moves.back().eval = 0;
                            }
                        }
                        else
                        {
                            try {
                                double eval = std::stod(evalStr);
                                moves.back().eval = static_cast<int>(eval * 100); // centipawns
                            }
                            catch (...) {
                                moves.back().eval = 0;
                            }
                        }
                    }
                }
                continue;
            }

            // Skip move numbers (e.g., "1.", "23.", "14...") and results
            if (token.empty())
                continue;
            if (std::isdigit(token[0]))
            {
                auto dotPos = token.find('.');
                if (dotPos != std::string::npos)
                    continue;
            }
            if (std::find(results.begin(), results.end(), token) != results.end())
                continue;
            if (token[0] == '(')
                continue; // skip variations (not robust, but fast)

            // Remove trailing comment/annotation symbols
            size_t end = token.find_first_of("!?#$");
            if (end != std::string::npos)
                token = token.substr(0, end);

            // Remove trailing '+' or '#' for check/checkmate detection
            bool isCheck = false, isCheckmate = false;
            if (!token.empty() && token.back() == '+')
            {
                isCheck = true;
                token.pop_back();
            }
            else if (!token.empty() && token.back() == '#')
            {
                isCheckmate = true;
                token.pop_back();
            }

            // Restore check/checkmate if both present (e.g., e8=Q+#)
            if (!token.empty() && (token.back() == '+' || token.back() == '#'))
            {
                if (token.back() == '+') isCheck = true;
                if (token.back() == '#') isCheckmate = true;
                token.pop_back();
            }

            if (!token.empty())
            {
                moves.push_back(PgnMove{
                    token,
                    ply++,
                    0, // eval will be set if a comment follows
                    isCheck,
                    isCheckmate,
                    false
                    });
            }
        }
        return moves;
    }


    std::vector<std::vector<PgnMove>> read_pgn_file(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file)
            throw std::runtime_error("Could not open PGN file: " + filename);

        std::vector<std::vector<PgnMove>> gamesMoves;
        std::ostringstream currentGame;
        bool inGame = false;
        std::string line;

        auto extract_moves_section = [](const std::string& gameText) -> std::string {
            std::istringstream iss(gameText);
            std::string line, movesSection;
            bool movesStarted = false;
            while (std::getline(iss, line)) {
                if (!movesStarted && (line.empty() || line[0] == '['))
                    continue;
                movesStarted = true;
                if (!line.empty())
                    movesSection += line + ' ';
            }
            return movesSection;
            };

        while (std::getline(file, line))
        {
            // Detect the start of a new game
            if (line.rfind("[Event", 0) == 0)
            {
                if (inGame)
                {
                    // Extract only the moves section and parse
                    std::string movesText = extract_moves_section(currentGame.str());
                    auto moves = parse_pgn_moves(movesText);
                    gamesMoves.push_back(std::move(moves));
                    currentGame.str("");
                    currentGame.clear();
                }
                inGame = true;
            }
            if (inGame)
            {
                currentGame << line << '\n';
            }
        }

        // Parse the last game if present
        if (inGame && !currentGame.str().empty())
        {
            std::string movesText = extract_moves_section(currentGame.str());
            auto moves = parse_pgn_moves(movesText);
            gamesMoves.push_back(std::move(moves));
        }

        return gamesMoves;
    }
}