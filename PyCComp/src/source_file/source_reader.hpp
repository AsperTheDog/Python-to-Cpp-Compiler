#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class SourceReader
{
public:
    struct ModuleFile
    {
        std::filesystem::path fileName;
        std::string fileContent;
        std::vector<uint32_t> dependencies;
        std::vector<std::string> stdDependencies;
    };

    explicit SourceReader(const std::filesystem::path& p_MainFile, const std::filesystem::path& p_WorkingDir = {});

    [[nodiscard]] uint32_t getModuleCount() const { return static_cast<uint32_t>(m_ModuleFiles.size()); }
    [[nodiscard]] const std::filesystem::path& getWorkingDir() const { return m_WorkingDir; }

    const ModuleFile* getModule(uint32_t p_Index) const;
    const ModuleFile* getModule(std::string_view p_FileName) const;
    [[nodiscard]] uint32_t getModuleIndex(std::string_view p_FileName) const;

private:
    uint32_t parseModule(const std::filesystem::path& p_FileName);

    std::vector<ModuleFile> m_ModuleFiles;
    std::filesystem::path m_WorkingDir;

    inline static const std::unordered_map<std::string, std::string> s_StdModules = {
        {"__future__", ""},
        {"math", "cmath"}
    };
};

