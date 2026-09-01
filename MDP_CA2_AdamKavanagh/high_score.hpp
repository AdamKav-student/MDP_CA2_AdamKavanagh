// Adam Kavanagh - D00247069
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct HighScoreEntry
{
    std::string m_name;
    uint16_t    m_score = 0;
};

// Game persistence. The server keeps a small plain-text table of the best
// individual kill counts it has ever seen and rewrites it at the end of every
// match, so records survive the server process being closed. The file is
// deliberately human-readable ("<score> <name>" per line) so it can be
// inspected or reset without a tool.
class HighScoreTable
{
public:
    explicit HighScoreTable(std::string filename = "high_scores.txt");

    void Load();
    void Save() const;

    // Returns true when the score was good enough to make the table.
    bool Submit(const std::string& name, uint16_t score);

    const std::vector<HighScoreEntry>& GetEntries() const;
    std::string GetSummary() const;

    static constexpr std::size_t kMaxEntries = 5;

private:
    std::string                 m_filename;
    std::vector<HighScoreEntry> m_entries;
};
