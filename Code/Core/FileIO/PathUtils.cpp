// PathUtils.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "PathUtils.h"
#include "Core/Strings/AStackString.h"
#include "Core/Math/Conversions.h"
#include "Core/Math/Random.h"


#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

// Exists
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::IsFolderPath( const AString & path )
{
    const size_t pathLen = path.GetLength();
    if ( pathLen > 0 )
    {
        const char lastChar = path[ pathLen - 1 ];

        // Handle both slash types so we cope with non-cleaned paths
        if ( ( lastChar == NATIVE_SLASH ) || ( lastChar == OTHER_SLASH ) )
        {
            return true;
        }
    }
    return false;
}

// IsFullPath
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::IsFullPath( const AString & path )
{
    #if defined( __WINDOWS__ )
        // full paths on Windows have a drive letter and colon, or are unc
        return ( ( path.GetLength() >= 2 && path[ 1 ] == ':' ) ||
                 path.BeginsWith( NATIVE_DOUBLE_SLASH ) );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        // full paths on Linux/OSX/IOS begin with a slash
        return path.BeginsWith( NATIVE_SLASH );
    #endif
}

// ArePathsEqual
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::ArePathsEqual( const AString & cleanPathA, const AString & cleanPathB )
{
    #if defined( __LINUX__ )
        // Case Sensitive
        return ( cleanPathA == cleanPathB );
    #endif

    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        // Case Insensitive
        return ( cleanPathA.CompareI( cleanPathB ) == 0 );
    #endif
}

// IsWildcardMatch
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::IsWildcardMatch( const char * pattern, const char * path )
{
    #if defined( __LINUX__ )
        // Linux : Case sensitive
        return AString::Match( pattern, path );
    #else
        // Windows & OSX : Case insensitive
        return AString::MatchI( pattern, path );
    #endif
}

// PathBeginsWith
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::PathBeginsWith( const AString & cleanPath, const AString & cleanSubPath )
{
    #if defined( __LINUX__ )
        // Linux : Case sensitive
        return cleanPath.BeginsWith( cleanSubPath );
    #else
        // Windows & OSX : Case insensitive
        return cleanPath.BeginsWithI( cleanSubPath );
    #endif
}

// PathEndsWithFile
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::PathEndsWithFile( const AString & cleanPath, const AString & fileName )
{
    // Work out if ends match
    #if defined( __LINUX__ )
        // Linux : Case sensitive
        const bool endMatch = cleanPath.EndsWith( fileName );
    #else
        // Windows & OSX : Case insensitive
        const bool endMatch = cleanPath.EndsWithI( fileName );
    #endif
    if ( !endMatch )
    {
        return false;
    }

    // if it's an entire match (a full path for example)
    if ( cleanPath.GetLength() == fileName.GetLength() )
    {
        return true;
    }

    // Sanity check - if fileName was longer then path (or equal) we can't get here
    ASSERT( cleanPath.GetLength() > fileName.GetLength() );
    const size_t potentialSlashIndex = ( cleanPath.GetLength() - fileName.GetLength() ) - 1; // -1 for char before match
    const char potentialSlash = cleanPath[ potentialSlashIndex ];
    if ( potentialSlash == NATIVE_SLASH )
    {
        return true; // full filename part matches (e.g. c:\thing\stuff.cpp | stuff.cpp )
    }
    return false; // fileName is only a partial match (e.g. c:\thing\otherstuff.cpp | stuff.cpp )
}

// EnsureTrailingSlash
//------------------------------------------------------------------------------
/*static*/ void PathUtils::EnsureTrailingSlash( AString & path )
{
    // check for existing slash
    const size_t pathLen = path.GetLength();
    if ( pathLen > 0 )
    {
        const char lastChar = path[ pathLen - 1 ];
        if ( lastChar == NATIVE_SLASH )
        {
            return; // good slash - nothing to do
        }
        if ( lastChar == OTHER_SLASH )
        {
            // bad slash, do fixup
            path[ pathLen - 1 ] = NATIVE_SLASH;
            return;
        }
    }

    // add slash
    path += NATIVE_SLASH;
}

// FixupFolderPath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::FixupFolderPath( AString & path )
{
    // Normalize slashes - TODO:C This could be optimized into one pass
    path.Replace( OTHER_SLASH, NATIVE_SLASH );
    #if defined( __WINDOWS__ )
        const bool isUNCPath = path.BeginsWith( NATIVE_DOUBLE_SLASH );
    #endif
    while( path.Replace( NATIVE_DOUBLE_SLASH, NATIVE_SLASH_STR ) ) {}

    #if defined( __WINDOWS__ )
        if ( isUNCPath )
        {
            AStackString<> copy( path );
            path.Clear();
            path += NATIVE_SLASH; // Restore double slash by adding one back
            path += copy;
        }
    #endif

    // ensure slash termination
    if ( path.EndsWith( NATIVE_SLASH ) == false )
    {
        path += NATIVE_SLASH;
    }
}

// FixupFilePath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::FixupFilePath( AString & path )
{
    // Normalize slashes - TODO:C This could be optimized into one pass
    path.Replace( OTHER_SLASH, NATIVE_SLASH );
    while( path.Replace( NATIVE_DOUBLE_SLASH, NATIVE_SLASH_STR ) ) {}

    // Sanity check - calling this function on a folder path is an error
    ASSERT( path.EndsWith( NATIVE_SLASH ) == false );
}

// StripFileExtension
//------------------------------------------------------------------------------
/*static*/ void PathUtils::StripFileExtension( AString & filePath )
{
    const char * lastDot = filePath.FindLast( '.' );
    if ( lastDot )
    {
        filePath.SetLength( (uint32_t)( lastDot - filePath.Get() ) );
    }
}

// GetRelativePath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::GetRelativePath( const AString & basePath,
                                            const AString & fileName,
                                            AString & outRelativeFileName )
{
    // Makes no sense to call with empty basePath
    ASSERT( basePath.IsEmpty() == false );

    // Can only determine relative paths if both are of the same scope
    ASSERT( IsFullPath( basePath ) == IsFullPath( fileName ) );

    // Handle base paths which are not slash terminated
    if ( basePath.EndsWith( NATIVE_SLASH ) == false )
    {
        AStackString<> basePathCopy( basePath );
        basePathCopy += NATIVE_SLASH;
        GetRelativePath( basePathCopy, fileName, outRelativeFileName );
        return;
    }

    // Find common sub-path
    const char * pathA = basePath.Get();
    const char * pathB = fileName.Get();
    const char * itA = pathA;
    const char * itB = pathB;
    char compA = *itA;
    char compB = *itB;

    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        // Windows & OSX: Case insensitive
        if ( ( compA >= 'A' ) && ( compA <= 'Z' ) )
        {
            compA = 'a' + ( compA - 'A' );
        }
        if ( ( compB >= 'A' ) && ( compB <= 'Z' ) )
        {
            compB = 'a' + ( compB - 'A' );
        }
    #endif

    while ( ( compA == compB ) && ( compA != '\0' ) )
    {
        const bool dirToken = ( ( *itA == '/' ) || ( *itA == '\\' ) );
        itA++;
        compA = *itA;
        itB++;
        compB = *itB;
        if ( dirToken )
        {
            pathA = itA;
            pathB = itB;
        }

        #if defined( __WINDOWS__ ) || defined( __OSX__ )
            // Windows & OSX: Case insensitive
            if ( ( compA >= 'A' ) && ( compA <= 'Z' ) )
            {
                compA = 'a' + ( compA - 'A' );
            }
            if ( ( compB >= 'A' ) && ( compB <= 'Z' ) )
            {
                compB = 'a' + ( compB - 'A' );
            }
        #endif
    }

    const bool hasCommonSubPath = ( pathA != basePath.Get() );
    if ( hasCommonSubPath == false )
    {
        // No common sub-path, so use fileName as-is
        outRelativeFileName = fileName;
        return;
    }

    // Build relative path

    // For every remaining dir in the project path, go up one directory
    outRelativeFileName.Clear();
    for ( ;; )
    {
        const char c = *pathA;
        if ( c == 0 )
        {
            break;
        }
        if ( ( c == '/' ) || ( c == '\\' ) )
        {
            #if defined( __WINDOWS__ )
                outRelativeFileName += "..\\";
            #else
                outRelativeFileName += "../";
            #endif
        }
        ++pathA;
    }

    // Add remainder of source path relative to the common sub path
    outRelativeFileName += pathB;
}

// GetBaseName
//------------------------------------------------------------------------------

/* static */ void PathUtils::GetBaseName( const AString& pathName, AString& baseName )
{
    if ( const char * slashPos = pathName.FindLast( NATIVE_SLASH ) )
    {
        baseName = slashPos + 1;
    }
    else
    {
        baseName = pathName;
    }

}

// GetDirectoryName
//------------------------------------------------------------------------------

/* static */ void PathUtils::GetDirectoryName( const AString& pathName, AString& dirName )
{
    if ( pathName.EndsWith( NATIVE_SLASH ) )
    {
        dirName = pathName;
        return;
    }
    if ( const char * slashPos = pathName.FindLast( NATIVE_SLASH ) )
    {
        dirName.Assign( pathName.Get(), slashPos + 1 ); // Include slash
    }
    else
    {
        dirName.Clear();
    }
}

// JoinPath
//------------------------------------------------------------------------------

/* static */ void PathUtils::JoinPath( AString& pathName, const AString& baseName ) {
    EnsureTrailingSlash( pathName );
    pathName += baseName;
}

namespace dtlto
{

    static int CompareFileName( const char * nameA,
                                const char * nameB )
    {
#ifdef __WINDOWS__
        return ::_stricmp( nameA, nameB );
#endif
#ifdef __LINUX__
        return ::strcmp( nameA, nameB );
#endif
    }

    static int CompareFileName( const AString & nameA,
                                const AString & nameB )
    {
        return CompareFileName( nameA.Get(), nameB.Get() );
    }
    struct DirectoryNode;

    struct FileNode
    {
      protected:
        DirectoryNode * parent = nullptr;
        AString name;
        uint64_t size = 0;
        uint32_t mode = 0;

      public:
        FileNode() = default;
        FileNode( const AString & file_name,
                  uint64_t file_size = 0,
                  uint32_t file_mode = 0 )
            : name( file_name ),
              size( file_size ),
              mode( file_mode )
        {}

        DirectoryNode * GetParent() const
        {
            return parent;
        }
        void SetParent( DirectoryNode * parent_ptr )
        {
            parent = parent_ptr;
        }

        const AString & GetName() const
        {
            return name;
        }
        int CompareName( const AString & file_name ) const
        {
            return CompareFileName( name, file_name );
        }
    };

    struct DirectoryNode
    {
      protected:
        DirectoryNode * parent = nullptr;
        Array< DirectoryNode * > childs;
        AString name;
        uint32_t mode = 0;
        Array< FileNode > files;

      public:
        DirectoryNode() = default;
        DirectoryNode( const AString & dir_name,
                       uint32_t dir_mode = 0 )
            : parent( nullptr ),
              name( dir_name ),
              mode( dir_mode )
        {}

        virtual ~DirectoryNode()
        {
            for ( DirectoryNode * dir : GetChilds() )
            {
                if ( dir )
                    delete dir;
            }
            name.Clear();
            files.Clear();
            childs.Clear();
        }

        int CompareName( const AString & dir_name ) const
        {
            return CompareFileName( name, dir_name );
        }

        // Constant getters.
        const AString & GetName() const
        {
            return name;
        }
        const DirectoryNode * GetParent() const
        {
            return parent;
        }
        void SetParent( DirectoryNode * parent_ptr )
        {
            parent = parent_ptr;
        }
        const Array< FileNode > & GetFiles() const
        {
            return files;
        }

        // Non-constant getters.
        DirectoryNode * GetParent()
        {
            return parent;
        }
        Array< DirectoryNode * > & GetChilds()
        {
            return childs;
        }
        Array< FileNode > & GetFiles()
        {
            return files;
        }

        FileNode * FindFile( const AString & file_name )
        {
            for ( FileNode & file : GetFiles() )
            {
                if ( 0 == file.CompareName( file_name ) )
                {
                    return &file;
                }
            }
            return nullptr;
        }

        FileNode * AddFile( const AString & file_name,
                            uint64_t file_size = 0,
                            uint32_t file_mode = 0 )
        {
            FileNode * file_node;
            if ( nullptr != ( file_node = FindFile( file_name ) ) )
                return file_node;
            FileNode fileNode( file_name, file_size, file_mode );
            fileNode.SetParent( this );
            GetFiles().Append( fileNode );
            return file_node;
        }

        DirectoryNode * FindChildDirectory( const AString & dir_name )
        {
            for ( DirectoryNode * dir : GetChilds() )
            {
                if ( 0 == dir->CompareName( dir_name ) )
                    return dir;
            }
            return nullptr;
        }

        const DirectoryNode * FindChildDirectory( const AString & dir_name,
                                                  size_t & idx )
        {
            idx = 0;
            for ( const DirectoryNode * dir : GetChilds() )
            {
                if ( 0 == dir->CompareName( dir_name ) )
                    return dir;
                ++idx;
            }
            return nullptr;
        }

        static DirectoryNode * Create( const AString & node_name,
                                       uint32_t node_mode = 0,
                                       DirectoryNode * parent_node = nullptr )
        {
            DirectoryNode * dir_node = new DirectoryNode( node_name, node_mode );
            if ( !dir_node )
                return nullptr;
            dir_node->SetParent( parent_node );
            return dir_node;
        }

        static DirectoryNode * Create( const char * name,
                                       uint32_t node_mode = 0,
                                       DirectoryNode * parent_node = nullptr )
        {
            AString node_name( name );
            return Create( node_name, node_mode, parent_node );
        }

        DirectoryNode * AddChildDirectory( const AString & dir_name,
                                           uint32_t dir_mode = 0 )
        {
            DirectoryNode * child_dir = nullptr;
            if ( nullptr != ( child_dir = FindChildDirectory( dir_name ) ) )
                return child_dir;
            child_dir = Create( dir_name, dir_mode, this );
            if ( !child_dir )
                return nullptr;
            GetChilds().Append( child_dir );
            return child_dir;
        }
    };

    struct PathParts
    {
        Array< AString > dirs;
        AString name;

        bool is_dir = false;
        bool is_absolute = false;

        size_t GetDirsSize() const
        {
            return dirs.GetSize();
        }
        const AString & GetDirsPart( size_t idx ) const
        {
            return ( idx < GetDirsSize() ) ? dirs[ idx ] : AString::GetEmpty();
        }
        void AddDirsPart( const char * s )
        {
            if ( s && *s != '\0' )
            {
                AString str( s );
                dirs.Append( str );
            }
        }
        const AString & GetFileName() const
        {
            return name;
        }
        void SetAbsolute()
        {
            is_absolute = true;
        }
        void SetFileName( const AString & file_name )
        {
            name = file_name;
            is_dir = false;
        }
        void SetFileName( const char * file_name )
        {
            AString s( file_name );
            SetFileName( s );
        }
        bool IsAbsolute() const
        {
            return is_absolute;
        }
    };

#define IS_SLASH( __c ) ( ( ( __c ) == NATIVE_SLASH ) || ( ( __c ) == OTHER_SLASH ) )
    bool BeginsWithSlash( const char * s )
    {
        if ( !s )
            return false;
        if ( ::strlen( s ) < 1 )
            return false;
        return IS_SLASH( *s );
    }

    bool IsSlash( char c )
    {
        bool yes = IS_SLASH( c );
        return yes;
    }
    bool IsDot( const AString & s )
    {
        if ( s.GetLength() != 1 )
            return false;
        return s[ 0 ] == '.';
    }
    bool IsDoubleDot( const AString & s )
    {
        if ( s.GetLength() != 2 )
            return false;
        return s[ 0 ] == '.' && s[ 1 ] == '.';
    }
#ifdef __WINDOWS__
    bool IsRootPath( const char * s )
    {
        if ( !s )
            return false;
        if ( ::strlen( s ) < 1 )
            return false;
        if ( BeginsWithSlash( s ) )
            return true;
        if ( ::strlen( s ) < 3 )
            return false;
        bool yes = ( isalpha( s[ 0 ] ) && s[ 1 ] == ':' && IsSlash( s[ 2 ] ) );
        return yes;
    }
#endif
#ifdef __LINUX__
    bool IsRootPath( const char * s )
    {
        if ( !s )
            return false;
        return IsSlash( *s );
    }
#endif

    void ParsePath( const AString & path,
                    bool isFile,
                    PathParts & path_parts )
    {
        struct ReadStream
        {
            AString s;
            size_t pos = 0;
            size_t top = 0, bottom = 0;
            ReadStream( const AString & in )
                : s( in ),
                  pos( 0 ),
                  top( 0 )
            {
                bottom = s.GetLength() == 0 ? 0 : s.GetLength() - 1;
            }

            size_t GetLength() const
            {
                return s.GetLength();
            }
            bool IsPosInRange()
            {
                return pos < GetLength();
            }

            size_t GetPos() const
            {
                return pos;
            }
            size_t GetPrevPos() const
            {
                return ( GetPos() == 0 ) ? 0 : GetPos() - 1;
            }
            const char * GetPtr( size_t idx = 0 ) const
            {
                return s.Get() + idx;
            }
            const char * GetCurPtr() const
            {
                return GetPtr( GetPos() );
            }
            char Get()
            {
                return IsPosInRange() ? s[ pos++ ] : '\0';
            }
            char UnGet()
            {
                return ( GetPos() == 0 ) ? '\0' : s[ --pos ];
            }
            char UnGet2()
            {
                UnGet();
                return UnGet();
            }
            char ReplaceChar( size_t idx,
                              char ch )
            {
                if ( idx <= GetLength() )
                {
                    char chp = s[ idx ];
                    s[ idx ] = ch;
                    return chp;
                }
                return '\0';
            }
        };

        ReadStream rs( path );
        char ch;
        const char * part_ptr = rs.GetPtr();

        if ( IsRootPath( part_ptr ) )
        {
#ifdef __WINDOWS__
            if ( BeginsWithSlash( part_ptr ) )
            {
                rs.Get(); // remove slash.
                path_parts.AddDirsPart( NATIVE_SLASH_STR );
            }
            else
            {
                rs.ReplaceChar( 2, '\0' );
                path_parts.AddDirsPart( part_ptr ); // add a part before slash
                rs.Get();                           // remove device letter.
                rs.Get();                           // remove ':'.
                rs.Get();                           // remove slash.
            }
#endif
#ifdef __LINUX__
            rs.Get(); // remove slash.
            path_parts.AddDirsPart( NATIVE_SLASH_STR );
#endif
            path_parts.SetAbsolute();
            part_ptr = rs.GetCurPtr(); // now part_ptr points to a symbol after slash.
        }

        while ( ( ch = rs.Get() ) != '\0' )
        {
            switch ( ch )
            {
            case NATIVE_SLASH:
            case OTHER_SLASH:
                rs.ReplaceChar( rs.GetPrevPos(), '\0' ); // replace slash for zero.
                path_parts.AddDirsPart( part_ptr );      // added a part before slash
                part_ptr = rs.GetCurPtr();               // now part_ptr points to a symbol after slash.
                break;
            case '.':
                // if pattern ../
                if ( rs.Get() != '.' )
                {
                    rs.UnGet();
                    break;
                }
                if ( !IsSlash( rs.Get() ) )
                {
                    rs.UnGet2();
                    break;
                }
                path_parts.AddDirsPart( ".." );
                part_ptr = rs.GetCurPtr(); // now part_ptr points to a symbol after slash.
                break;

            default: break;
            }
        }
        if ( isFile )
            path_parts.SetFileName( part_ptr );
        else
            path_parts.AddDirsPart( part_ptr );
    }

    struct FileSystemImpl
    {
        DirectoryNode * root = nullptr;
        DirectoryNode * cwd = nullptr;
        DirectoryNode * GetCurrentDirectoryNode()
        {
            return cwd;
        }

        virtual ~FileSystemImpl()
        {
            if ( root )
                delete root;
            if ( cwd )
                delete cwd;
        }

        DirectoryNode * CreateRootDirectoryNode()
        {
            if ( root )
                return root;
            root = DirectoryNode::Create( NATIVE_SLASH_STR );
            if ( !root )
                return nullptr;
            root->SetParent( nullptr );
            return root;
        }

        DirectoryNode * CreateCurrentDirectoryNode()
        {
            if ( cwd )
                return cwd;
            cwd = DirectoryNode::Create( "." );
            if ( !cwd )
                return nullptr;
            return cwd;
        }

        // #undef RANDOM_NAME_FOR_PARENT_DIR

        bool CreateDirectory( const PathParts & path_parts,
                              DirectoryNode *& dir_node )
        {
            DirectoryNode * cur_dir = nullptr;
            size_t idx = 0;
            dir_node = nullptr;
#ifdef RANDOM_NAME_FOR_PARENT_DIR
            Random rand_gen;
#endif
            if ( path_parts.IsAbsolute() )
            {
                if ( nullptr == ( cur_dir = CreateRootDirectoryNode() ) )
                    return false;
                idx++;
            }
            else
            {
                if ( nullptr == ( cur_dir = CreateCurrentDirectoryNode() ) )
                    return false;
            }

            for ( ; idx < path_parts.GetDirsSize(); ++idx )
            {
                const AString & path_part = path_parts.GetDirsPart( idx );
                if ( IsDot( path_part ) )
                {
                    continue;
                }
                else if ( IsDoubleDot( path_part ) )
                {
                    DirectoryNode * parent_dir;
                    parent_dir = cur_dir->GetParent();
                    if ( !parent_dir )
                    {
#ifdef RANDOM_NAME_FOR_PARENT_DIR
                        AString rand_str;
                        rand_gen.GetRandString( rand_str );
                        parent_dir = DirectoryNode::Create( rand_str );
#else
                        parent_dir = DirectoryNode::Create( ".." );
#endif
                        if ( !parent_dir )
                            return false;
                        parent_dir->GetChilds().Append( cur_dir );
                        cur_dir->SetParent( parent_dir );
                    }
                    cur_dir = parent_dir;
                }
                else
                {
                    if ( nullptr == ( cur_dir = cur_dir->AddChildDirectory( path_part ) ) )
                        return false;
                }
            }
            dir_node = cur_dir;
            return true;
        }

        bool OpenDirectory( const PathParts & path_parts,
                            DirectoryNode *& dir_node )
        {
            DirectoryNode * cur_dir = nullptr;
            size_t idx = 0;
            dir_node = nullptr;

            if ( path_parts.IsAbsolute() )
            {
                if ( nullptr == ( cur_dir = CreateRootDirectoryNode() ) )
                    return false;
                idx++;
            }
            else
            {
                if ( nullptr == ( cur_dir = CreateCurrentDirectoryNode() ) )
                    return false;
            }

            for ( ; idx < path_parts.GetDirsSize(); ++idx )
            {
                const AString & path_part = path_parts.GetDirsPart( idx );
                if ( IsDot( path_part ) )
                {
                    continue;
                }
                else if ( IsDoubleDot( path_part ) )
                {
                    DirectoryNode * parent_dir = cur_dir->GetParent();
                    if ( !parent_dir )
                    {
                        return false;
                    }
                    cur_dir = parent_dir;
                }
                else
                {
                    if ( nullptr == ( cur_dir = cur_dir->FindChildDirectory( path_part ) ) )
                        return false;
                }
            }
            dir_node = cur_dir;
            return true;
        }

        bool CreateFile( const PathParts & path_parts,
                         DirectoryNode *& dir_node,
                         const FileNode *& file_node )
        {
            if ( !CreateDirectory( path_parts, dir_node ) )
                return false;
            file_node = dir_node->AddFile( path_parts.GetFileName() );
            return true;
        }

        bool CreateFile( const AString & file_path )
        {
            PathParts path_parts;
            ParsePath( file_path, true, path_parts );
            DirectoryNode * dir_node;
            const FileNode * file_node;
            return CreateFile( path_parts, dir_node, file_node );
        }

        bool OpenFile( const AString & file_path,
                       DirectoryNode *& dir_node,
                       FileNode *& file_node )
        {
            PathParts path_parts;
            ParsePath( file_path, true, path_parts );
            if ( !OpenDirectory( path_parts, dir_node ) )
                return false;
            file_node = dir_node->FindFile( path_parts.GetFileName() );
            return true;
        }
        bool MakeCanonicalFilePath( const AString & file_path,
                                    AString & out_path )
        {
            PathParts path_parts;
            ParsePath( file_path, true, path_parts );
            DirectoryNode * dir_node = nullptr;
            const FileNode * file_node = nullptr;
            if ( !CreateFile( path_parts, dir_node, file_node ) )
                return false;
            Array< AString > out_path_parts;
            out_path_parts.SetCapacity( path_parts.GetDirsSize() + 1 );
            DirectoryNode * cur_node = dir_node;
            while ( cur_node )
            {
                out_path_parts.EmplaceBack( cur_node->GetName() );
                cur_node = cur_node->GetParent();
            }
            for ( int64_t idx = (int64_t)out_path_parts.GetSize() - 1; idx > 0; --idx )
            {
                out_path += out_path_parts[ (size_t)idx ];
                out_path += NATIVE_SLASH_STR;
            }
            out_path += file_node->GetName();
            return true;
        }

        void LevelsUpFromCurrentDirectoryNode( const DirectoryNode * dir_node,
                                               int32_t & levels_up )
        {
            const DirectoryNode * cur_node = dir_node;
            bool up_of_cwd = false;
            levels_up = 0;
            while ( cur_node )
            {
                if ( up_of_cwd )
                    ++levels_up;
                if ( !up_of_cwd && cur_node == GetCurrentDirectoryNode() )
                    up_of_cwd = true;
                cur_node = cur_node->GetParent();
            }
        }
    };
} // namespace dtlto

//
//
//
bool AnalyzeFilePaths( const Array< AString > & filePaths,
                       Array< AString > & canonicalFilePaths,
                       int32_t & numOfAbsPaths,
                       int32_t & levelsUpFromCurrentDir )
{
    levelsUpFromCurrentDir = 0;
    numOfAbsPaths = 0;
    dtlto::FileSystemImpl fs;

    for ( const AString & cur_path : filePaths )
    {
        dtlto::DirectoryNode * dir_node;
        bool rc = fs.CreateFile( cur_path );
        if ( !rc )
            return false;

        dtlto::FileNode * file_node;
        rc = fs.OpenFile( cur_path, dir_node, file_node );
        if ( !rc )
            return false;

        AString canonical_path;
        rc = fs.MakeCanonicalFilePath( cur_path, canonical_path );
        if ( !rc )
            return false;

        canonicalFilePaths.Append( canonical_path );
        if ( dtlto::IsRootPath( canonical_path.Get() ) )
            numOfAbsPaths++;

        int32_t levels_up;
        fs.LevelsUpFromCurrentDirectoryNode( dir_node, levels_up );
        levelsUpFromCurrentDir = Math::Max( levelsUpFromCurrentDir, levels_up );
    }
    return true;
}


// Local dir doesn't match remote directory.
// If all the files have one common ancestor node in the file system and all the paths are relative to a current directory, 
// then we can create a sub-dir in the remote directory that would mimic the location of the files in the local file system.
