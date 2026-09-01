// Adam Kavanagh - D00247069
#include "high_score.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

HighScoreTable::HighScoreTable(std::string filename)
    : m_filename(std::move(filename))
    , m_entries()
{
    Load();
}

void HighScoreTable::Load()
{
    m_entries.clear();

    std::ifstream input(m_filename);
    if (!input)
    {
        // No file yet - the first Save() will create one. A missing table is
        // not an error, it just means no match has finished on this server.
        return;
    }

    std::string line;
    while (std::getline(input, line) && m_entries.size() < kMaxEntries)
    {
        std::istringstream stream(line);
        HighScoreEntry entry;
        unsigned int score = 0;

        if (stream >> score && std::getline(stream >> std::ws, entry.m_name) && !entry.m_name.empty())
        {
            entry.m_score = static_cast<uint16_t>(score);
            m_entries.push_back(entry);
        }
    }

    std::sort(m_entries.begin(), m_entries.end(),
        [](const HighScoreEntry& lhs, const HighScoreEntry& rhs) { return lhs.m_score > rhs.m_score; });
}

void HighScoreTable::Save() const
{
    std::ofstream output(m_filename, std::ios::trunc);
    if (!output)
    {
        return;
    }

    for (const HighScoreEntry& entry : m_entries)
    {
        output << entry.m_score << ' ' << entry.m_name << '\n';
    }
}

bool HighScoreTable::Submit(const std::string& name, uint16_t score)
{
    if (score == 0)
    {
        return false;
    }

    m_entries.push_back(HighScoreEntry{ name, score });
    std::stable_sort(m_entries.begin(), m_entries.end(),
        [](const HighScoreEntry& lhs, const HighScoreEntry& rhs) { return lhs.m_score > rhs.m_score; });

    const bool made_table = std::find_if(m_entries.begin(),
        m_entries.begin() + std::min(m_entries.size(), kMaxEntries),
        [&](const HighScoreEntry& entry) { return entry.m_name == name && entry.m_score == score; }) != m_entries.begin() + std::min(m_entries.size(), kMaxEntries);

    if (m_entries.size() > kMaxEntries)
    {
        m_entries.resize(kMaxEntries);
    }

    return made_table;
}

const std::vector<HighScoreEntry>& HighScoreTable::GetEntries() const
{
    return m_entries;
}

std::string HighScoreTable::GetSummary() const
{
    if (m_entries.empty())
    {
        return "No record set yet - first kill takes it";
    }

    std::ostringstream stream;
    stream << "Record to beat: " << m_entries.front().m_score << " kills by " << m_entries.front().m_name;
    return stream.str();
}
