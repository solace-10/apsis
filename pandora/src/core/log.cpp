// Copyright 2022 Pedro Nunes
//
// This file is part of Genesis.
//
// Genesis is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Genesis is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Genesis. If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>

#ifdef _WIN32
#include "windows.h"
#endif

#include "core/log.hpp"

//#include <SDL.h>

namespace WingsOfSteel::Pandora
{

//////////////////////////////////////////////////////////////////////////
// Log
//////////////////////////////////////////////////////////////////////////

std::mutex Log::m_Mutex;
Log::LogTargetList Log::m_Targets;

void Log::AddLogTarget( LogTargetSharedPtr pLogTarget )
{
    std::lock_guard<std::mutex> lock( m_Mutex );
    m_Targets.push_back( pLogTarget );
}

void Log::RemoveLogTarget( LogTargetSharedPtr pLogTarget )
{
    std::lock_guard<std::mutex> lock( m_Mutex );
    m_Targets.remove( pLogTarget );
}

Log::Stream Log::Info()
{
    return Log::Stream( Log::Level::Info );
}

Log::Stream Log::Warning()
{
    return Log::Stream( Log::Level::Warning );
}

Log::Stream Log::Error()
{
    return Log::Stream( Log::Level::Error );
}

// Internal logging function. Should only be called by Log::Stream's destructor.
void Log::LogInternal( const std::string& text, Log::Level level )
{
    std::lock_guard<std::mutex> lock( m_Mutex );
    for ( auto& pTarget : m_Targets )
    {
        pTarget->Log( text, level );
    }
}

//////////////////////////////////////////////////////////////////////////
// Log::Stream
//////////////////////////////////////////////////////////////////////////

Log::Stream::Stream( Log::Level level )
{
    m_Level = level;
}

Log::Stream::~Stream()
{
    Log::LogInternal( m_Collector.str(), m_Level );
}

//////////////////////////////////////////////////////////////////////////
// ILogTarget
// Prints the message to the console.
//////////////////////////////////////////////////////////////////////////

const std::string& ILogTarget::GetPrefix( Log::Level level )
{
    static std::string prefixes[ static_cast<size_t>( Log::Level::Count ) ] = { "[INFO] ", "[WARNING] ", "[ERROR] " };

    return prefixes[ static_cast<size_t>( level ) ];
}

//////////////////////////////////////////////////////////////////////////
// StdOutLogger
// Prints the message to std::out.
//////////////////////////////////////////////////////////////////////////

void StdOutLogger::Log( const std::string& text, Log::Level type )
{
    std::cout << GetPrefix( type ) << text << std::endl;
}

//////////////////////////////////////////////////////////////////////////
// FileLogger
//////////////////////////////////////////////////////////////////////////

#if defined(TARGET_PLATFORM_NATIVE)
FileLogger::FileLogger( const std::filesystem::path& filePath )
{
    m_File.open( filePath, std::fstream::out | std::fstream::trunc );
}

FileLogger::~FileLogger()
{
    if ( m_File.is_open() )
    {
        m_File.close();
    }
}

void FileLogger::Log( const std::string& text, Log::Level type )
{
    if ( m_File.is_open() )
    {
        m_File << GetPrefix( type ) << text << std::endl;
        m_File.flush();
    }
}
#endif

//////////////////////////////////////////////////////////////////////////
// MessageBoxLogger
//////////////////////////////////////////////////////////////////////////

#if defined(TARGET_PLATFORM_NATIVE)
void MessageBoxLogger::Log( const std::string& text, Log::Level type )
{
    if ( type == Log::Level::Warning )
    {
        //SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_WARNING, "Warning", text.c_str(), nullptr );
    }
    else if ( type == Log::Level::Error )
    {
        //SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Error", text.c_str(), nullptr );
    }
}
#endif

} // namespace WingsOfSteel::Pandora