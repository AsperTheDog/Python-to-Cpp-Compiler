#include "source_reader.hpp"

#include <filesystem>
#include <fstream>
#include <unordered_set>

SourceReader::SourceReader(const std::filesystem::path& p_MainFile, const std::filesystem::path& p_WorkingDir)
{
    if (p_WorkingDir.empty())
    {
        const std::filesystem::path l_AbsPath = std::filesystem::absolute(p_MainFile);
        m_WorkingDir = l_AbsPath.parent_path();
    }
    else
    {
        m_WorkingDir = std::filesystem::absolute(p_WorkingDir);
    }
    parseModule(p_WorkingDir / p_MainFile);
}

const SourceReader::ModuleFile* SourceReader::getModule(const uint32_t p_Index) const
{
    if (p_Index < m_ModuleFiles.size())
    {
        return &m_ModuleFiles[p_Index];
    }
    return nullptr;
}

const SourceReader::ModuleFile* SourceReader::getModule(const std::string_view p_FileName) const
{
    const uint32_t l_Index = getModuleIndex(p_FileName);
    if (l_Index != UINT32_MAX)
    {
        return &m_ModuleFiles[l_Index];
    }
    return nullptr;
}

uint32_t SourceReader::getModuleIndex(const std::string_view p_FileName) const
{
    for (uint32_t l_Index = 0; l_Index < m_ModuleFiles.size(); ++l_Index)
    {
        if (m_ModuleFiles[l_Index].fileName.filename() == p_FileName)
        {
            return l_Index;
        }
    }
    return UINT32_MAX;
}

uint32_t SourceReader::parseModule(const std::filesystem::path& p_FileName)
{
    uint32_t l_Index = getModuleIndex(p_FileName.filename().string());
    if (l_Index != UINT32_MAX)
    {
        return l_Index;
    }

    std::ifstream l_File(p_FileName.c_str());
    if (!l_File.is_open())
    {
        throw std::runtime_error("Could not open file: " + p_FileName.string());
    }

    ModuleFile l_Module;
    l_Module.fileName = p_FileName;

    std::unordered_set<std::string> l_Dependencies;

    std::string l_Line;
    bool l_Importing = true;
    while (std::getline(l_File, l_Line))
    {
        if (l_Line.empty())
        {
            continue;
        }

        if (l_Line.starts_with("import") || l_Line.starts_with("from"))
        {
            std::string moduleName = l_Line.substr(l_Line.find_first_of(' ') + 1);
            moduleName = moduleName.substr(0, moduleName.find_first_of(' '));
            std::ranges::replace(moduleName, '.', '/');
            l_Dependencies.insert(moduleName);

            if (!l_Importing)
            {
                throw std::runtime_error("Import statement found outside of import section in file: " + p_FileName.string() + ".\nAll imports in a module must be at the beginning of the file.");
            }
        }
        else
        {
            l_Module.fileContent += l_Line + '\n';
            l_Importing = false;
        }
    }

    for (const std::string& l_Dep : l_Dependencies)
    {
        std::filesystem::path depPath = m_WorkingDir / (l_Dep + ".py");
        if (s_StdModules.contains(l_Dep))
        {
            const std::string& l_ModuleName = s_StdModules.at(l_Dep);
            if (!l_ModuleName.empty())
            {
                l_Module.stdDependencies.push_back(s_StdModules.at(l_Dep));
            }
        }
        else
        {
            if (!std::filesystem::exists(depPath))
            {
                std::filesystem::path l_LocalDir = p_FileName.parent_path();
                depPath = l_LocalDir / (l_Dep + ".py");
            }
            l_Module.dependencies.push_back(parseModule(depPath));
        }
    }

    m_ModuleFiles.push_back(std::move(l_Module));
    return m_ModuleFiles.size() - 1;
}
