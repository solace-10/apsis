#pragma once

namespace WingsOfSteel
{
class Database;
}

namespace WingsOfSteel::Private
{

class DatabaseImpl
{
public:
    DatabaseImpl(Database& parent) : m_Parent(parent) {}
    virtual ~DatabaseImpl() {}

    virtual void GetAllObjects() = 0;

protected:
    Database& m_Parent;
};

} // namespace WingsOfSteel::Private
