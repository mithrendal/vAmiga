// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Command.h"
#include <algorithm>

void
Command::add(const std::vector<string> &tokens,
             const string &type,
             const string &help)
{
    add(tokens, type, help, nullptr, 0);
}

void
Command::add(const std::vector<string> &tokens,
             const string &type,
             const string &help,
             void (RetroShell::*action)(Arguments&, long),
             isize numArgs, long param)
{
    add(tokens, type, help, action, { numArgs, numArgs }, param);
}

void
Command::add(const std::vector<string> &tokens,
             const string &type,
             const string &help,
             void (RetroShell::*action)(Arguments&, long),
             std::pair <isize,isize> numArgs,
             long param)
{
    assert(!tokens.empty());
    
    // Traverse the node tree
    Command *cmd = seek(std::vector<string> { tokens.begin(), tokens.end() - 1 });
    assert(cmd != nullptr);
    
    // Create instruction
    Command d;
    d.parent = this;
    d.token = tokens.back();
    d.type = type;
    d.info = help;
    d.action = action;
    d.minArgs = numArgs.first;
    d.maxArgs = numArgs.second;
    
    // Register instruction
    cmd->args.push_back(d);
}

void
Command::remove(const string& token)
{
    for(auto it = std::begin(args); it != std::end(args); ++it) {
        if (it->token == token) { args.erase(it); return; }
    }
}

Command *
Command::seek(const string& token)
{
    for (auto &it : args) {
        if (it.token == token) return &it;
    }
    return nullptr;
}

Command *
Command::seek(const std::vector<string> &tokens)
{
    Command *result = this;
    
    for (auto &it : tokens) {
        if ((result = result->seek(it)) == nullptr) break;
    }
    
    return result;
}

std::vector<string>
Command::types() const
{
    std::vector<string> result;
    
    for (auto &it : args) {
        
        if (it.hidden) continue;
        
        if (std::find(result.begin(), result.end(), it.type) == result.end()) {
            result.push_back(it.type);
        }
    }
    
    return result;
}

std::vector<const Command *>
Command::filterType(const string& type) const
{
    std::vector<const Command *> result;
    
    for (auto &it : args) {
        
        if (it.hidden) continue;
        if (it.type == type) result.push_back(&it);
    }
    
    return result;
}

std::vector<const Command *>
Command::filterPrefix(const string& prefix) const
{
    std::vector<const Command *> result;
    
    for (auto &it : args) {
        
        if (it.hidden) continue;
        if (it.token.substr(0, prefix.size()) == prefix) result.push_back(&it);
    }

    return result;
}

string
Command::autoComplete(const string& token)
{
    string result = token;
    
    auto matches = filterPrefix(token);
    if (!matches.empty()) {
        
        const Command *first = matches.front();
        for (auto i = token.size(); i < first->token.size(); i++) {
            
            for (auto m: matches) {
                if (m->token.size() <= i || m->token[i] != first->token[i]) {
                    return result;
                }
            }
            result += first->token[i];
        }
    }
    return result;
}

string
Command::tokens() const
{
    string result = this->parent ? this->parent->tokens() : "";
    return result == "" ? token : result + " " + token;
}

string
Command::usage() const
{
    string firstArg, otherArgs;
    
    if (args.empty()) {

        firstArg = maxArgs == 0 ? "" : maxArgs == 1 ? "<value>" : "<values>";
        if (minArgs == 0) firstArg = "[" + firstArg + "]";
        
    } else {
        
        // Collect all argument types
        auto t = types();
        
        // Describe the first argument
        for (usize i = 0; i < t.size(); i++) {
            firstArg += (i == 0 ? "" : "|") + t[i];
        }
        firstArg = "<" + firstArg + ">";
        
        // Describe the remaining arguments (if any)
        bool printArg = false, printOpt = false;
        for (auto &it : args) {
            if (it.action != nullptr && it.minArgs == 0) printOpt = true;
            if (it.maxArgs > 0 || !it.args.empty()) printArg = true;
        }
        if (printArg) {
            otherArgs = printOpt ? "[<arguments>]" : "<arguments>";
        }
    }
    
    return tokens() + " " + firstArg + " " + otherArgs;
}
