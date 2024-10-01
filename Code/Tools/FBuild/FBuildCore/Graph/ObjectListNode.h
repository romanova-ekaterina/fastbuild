// ObjectListNode.h - manages a list of ObjectNodes
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// FBuild
#include <Tools/FBuild/FBuildCore/Graph/ObjectNode.h>

// Core
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Args;
class CompilerNode;
class Function;
class NodeGraph;

// ObjectListNode
//------------------------------------------------------------------------------
class ObjectListNode : public Node
{
    REFLECT_NODE_DECLARE( ObjectListNode )
public:
    ObjectListNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~ObjectListNode() override;

    static inline Node::Type GetTypeS() { return Node::OBJECT_LIST_NODE; }

    virtual bool IsAFile() const override;

    const char * GetObjExtension() const;

    void GetInputFiles( bool objectsInsteadOfLibs, Array<AString> & outInputs ) const;
    void GetInputFiles( Array< AString > & files ) const;

    inline const AString & GetCompilerOutputPath() const { return m_CompilerOutputPath; }
    inline const AString & GetCompilerOptions() const { return m_CompilerOptions; }
    inline const AString & GetCompiler() const { return m_Compiler; }

    void GetObjectFileName( const AString & fileName, const AString & baseDir, AString & objFile );

    void EnumerateInputFiles( void (*callback)( const AString & inputFile, const AString & baseDir, void * userData ), void * userData ) const;

protected:
    friend class FunctionObjectList;

    virtual bool GatherDynamicDependencies( NodeGraph & nodeGraph );
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph ) override;
    virtual BuildResult DoBuild( Job * job ) override;

    // internal helpers
    bool CreateDynamicObjectNode( NodeGraph & nodeGraph,
                                  const AString & inputFileName,
                                  const AString & baseDir,
                                  const AString& outputFile,
                                  const AString& thinltoSummaryIndex,
                                  const AString& thinltoModuleId,
                                  const Array<AString>& thinltoImportsList,
                                  bool isUnityNode = false,
                                  bool isIsolatedFromUnityNode = false );
    ObjectNode * CreateObjectNode( NodeGraph & nodeGraph,
                                   const BFFToken * iter,
                                   const Function * function,
                                   const ObjectNode::CompilerFlags flags,
                                   const ObjectNode::CompilerFlags preprocessorFlags,
                                   const AString & compilerOptions,
                                   const AString & compilerOptionsDeoptimized,
                                   const AString & preprocessor,
                                   const AString & preprocessorOptions,
                                   const AString & objectName,
                                   const AString & objectInput,
                                   const AString & pchObjectName,
                                   const AString& thinltoSummaryIndexFile,
                                   const AString& thinltoModuleId,
                                   const Array < AString>& thinltoImportsList );

    // Exposed Properties
    AString             m_Compiler;
    AString             m_CompilerOptions;
    AString             m_CompilerOptionsDeoptimized;
    AString             m_CompilerOutputPath;
    AString             m_CompilerOutputPrefix;
    AString             m_CompilerOutputExtension;
    Array< AString >    m_CompilerInputPath;
    Array< AString >    m_CompilerInputPattern;
    Array< AString >    m_CompilerInputExcludePath;
    Array< AString >    m_CompilerInputExcludedFiles;
    Array< AString >    m_CompilerInputExcludePattern;
    Array< AString >    m_CompilerInputFiles;
    Array< AString >    m_CompilerInputUnity;
    AString             m_CompilerInputFilesRoot;
    Array< AString >    m_CompilerInputObjectLists;
    Array< AString >    m_CompilerForceUsing;
    bool                m_CompilerInputAllowNoFiles         = false;
    bool                m_CompilerInputPathRecurse          = true;
    bool                m_CompilerOutputKeepBaseExtension   = false;
    bool                m_DeoptimizeWritableFiles           = false;
    bool                m_DeoptimizeWritableFilesWithToken  = false;
    bool                m_AllowDistribution                 = true;
    bool                m_AllowCaching                      = true;
    AString             m_PCHInputFile;
    AString             m_PCHOutputFile;
    AString             m_PCHOptions;
    AString             m_Preprocessor;
    AString             m_PreprocessorOptions;
    Array< AString >    m_PreBuildDependencyNames;
    AString             m_ConcurrencyGroupName;

    // DTLTO
    Array< AString >    m_CompilerOutputFiles;        // Array of output object files.
    AString             m_CompilerOptionsBitcode;     // Compiler options for IR bitcode code generation.
    Array< AString >    m_ThinltoSummaryIndexFiles;   // Array of ThinLTO summary index files.
    Array< AString >    m_ThinltoImportFiles;         // Array of ThinLTO imports list files.
    Array< AString >    m_ThinltoImports;             // Array of strings. Each string contains a list of imports separated by semicolon.
    Array< AString >    m_ThinltoModuleIds;           // Array of strings. Each string contains a Module ID for an input file.
    AString             m_CompilerOptionModuleIdMap;  // Compiler option to specify a path to a Module ID map file.

    // Internal State
    AString             m_PrecompiledHeaderName;
    #if defined( __WINDOWS__ )
        AString             m_PrecompiledHeaderCPPFile;
    #endif
    AString             m_ExtraPDBPath;
    AString             m_ExtraASMPath;
    AString             m_ExtraSourceDependenciesPath;
    uint32_t            m_ObjectListInputStartIndex         = 0;
    uint32_t            m_ObjectListInputEndIndex           = 0;
    ObjectNode::CompilerFlags   m_CompilerFlags;
    ObjectNode::CompilerFlags   m_PreprocessorFlags;
};

//------------------------------------------------------------------------------
