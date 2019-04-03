#include "sqlitedatabase.h"

SQLiteDatabase::SQLiteDatabase(const QString &databasePath) :
	m_valid(true)
{
	connect(this, &SQLiteDatabase::sendMessage, g_mainWindow, &CrowdedHellGUI::sendMessage);

	QRandomGenerator randomGenerator(unsigned(QTime(0,0,0).secsTo(QTime::currentTime())));

	do
		m_connectionName = "_C_H_D_B_" + QString::number((randomGenerator.generate() & std::numeric_limits<qint32>::max()));
	while(QSqlDatabase::database(m_connectionName).isValid());

	QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
	database.setDatabaseName(databasePath);
	database.setHostName("localhost");
	if(!database.open())
	{
		sendMessage(MessageType::Error, "SQLite", tr("Open the database file failed, at : \"%1\".").arg(databasePath));
		m_valid = false;
	}
};

SQLiteDatabase::~SQLiteDatabase()
{
	close();
	QSqlDatabase::removeDatabase(m_connectionName);
};

bool SQLiteDatabase::isValid()
{
	return m_valid;
};

QStringList SQLiteDatabase::getAllTableNames()
{
	return QSqlDatabase::database(m_connectionName).tables();
};

bool SQLiteDatabase::createTable(const SQLiteDatabase::DataTable &table)
{
	if(!isValid())
	{
		sendMessage(MessageType::Error, "SQLite", tr("Database is not valid."));
		return false;
	}

	if(exists(table.tableName))
	{
		sendMessage(MessageType::Warning, "SQLite", tr("Table \"%1\" is already exists, so it won't be created again.").arg(table.tableName));
		return false;
	}

	if(table.columns.size() == 0)
	{
		sendMessage(MessageType::Error, "SQLite", tr("Cannot create table \"%1\" because it has no column.").arg(table.tableName));
		return false;
	}

	QString sqlSentence = "CREATE TABLE IF NOT EXISTS `";
	sqlSentence += table.tableName;
	sqlSentence += "`(";
	if(table.hasAutoIncrementIndex)
		sqlSentence += "_AutoIndex INTEGER PRIMARY KEY AUTO INCREMENT,";

	foreach(auto column, table.columns)
	{
		sqlSentence += QString("`") + column.first + QString("`");
		switch(column.second)
		{
			case DataType::INTEGER:
				sqlSentence += " INTEGER ";
			break;

			case DataType::REAL:
				sqlSentence += "REAL ";
			break;

			case DataType::TEXT:
				sqlSentence += "TEXT ";
			break;
		}

		if(column.first == table.primaryKey)
			sqlSentence += "PRIMARY KEY";
		sqlSentence += ",";
	}
	sqlSentence.replace(sqlSentence.length() - 1, 1, QChar(')'));

	QSqlQuery query(QSqlDatabase::database(m_connectionName));
	if(!query.exec(sqlSentence))
	{
		sendMessage(MessageType::Error, "SQLite", tr("Error occurred : %1").arg(query.lastError().text()));
		return false;
	}

	return true;
}

bool SQLiteDatabase::insert(const QString &tableName, const QVariantList &record)
{
	if(!isValid())
	{
		sendMessage(MessageType::Error, "SQLite", tr("Database is not valid."));
		return false;
	}

	if(exists(tableName))
	{
		sendMessage(MessageType::Warning, "SQLite", tr("Table \"%1\" is already exists, so it won't be created again.").arg(tableName));
		return false;
	}

	QString sqlSentence = "INSERT OR REPLACE INTO `";
	sqlSentence += tableName;
	sqlSentence += "` VALUES (";
	for (int i = 0; i < record.size(); i++)
		sqlSentence += "?,";
	sqlSentence.replace(sqlSentence.length() - 1, 1, QChar(')'));

	QSqlQuery query(QSqlDatabase::database(m_connectionName));
	if(!query.prepare(sqlSentence))
	{
		sendMessage(MessageType::Error, "SQLite", tr("Error occurred : %1").arg(query.lastError().text()));
		return false;
	}

	foreach(auto value, record)
		query.addBindValue(value);

	if(!query.execBatch())
	{
		sendMessage(MessageType::Error, "SQLite", tr("Error occurred : %1").arg(query.lastError().text()));
		return false;
	}

	return true;
}

bool SQLiteDatabase::update(const DataTable &table, const QString &primaryKeyValue, const QString &columnName, const QVariant &value)
{
	QString sqlSentence = "UPDATE `%1` SET `%2`=? WHERE `%3`=%4";
	sqlSentence = sqlSentence.arg(table.tableName).arg(columnName).arg(table.primaryKey.isEmpty() ? QString("_AutoIndex") : table.primaryKey).arg(primaryKeyValue);

	QSqlQuery query(QSqlDatabase::database(m_connectionName));
	if(!query.prepare(sqlSentence))
	{
		sendMessage(MessageType::Error, "SQLite", tr("Error occurred : %1").arg(query.lastError().text()));
		return false;
	}

	query.addBindValue(value);
	if(!query.execBatch())
	{
		sendMessage(MessageType::Error, "SQLite", tr("Error occurred : %1").arg(query.lastError().text()));
		return false;
	}

	return true;
};

bool SQLiteDatabase::exists(const QString &tableName)
{
	return getAllTableNames().contains(tableName);
};

void SQLiteDatabase::close()
{
	QSqlDatabase::database(m_connectionName).close();
	m_valid = false;
};