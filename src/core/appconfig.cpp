#include "core/appconfig.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QCoreApplication>

namespace mc {

AppConfig &AppConfig::instance()
{
    static AppConfig inst;
    return inst;
}

void AppConfig::load(const QString &path)
{
    configPath_ = path.isEmpty()
        ? QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("config.json"))
        : path;

    QFile f(configPath_);
    if (!f.exists()) {
        // 首次运行：用默认值保存一份模板
        saveAs(configPath_);
        return;
    }

    if (!f.open(QIODevice::ReadOnly)) {
        data_ = AppConfigData{}; // 回退默认
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    fromJson(doc.object());
}

void AppConfig::save()
{
    if (configPath_.isEmpty())
        configPath_ = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("config.json"));
    saveAs(configPath_);
}

void AppConfig::saveAs(const QString &path)
{
    configPath_ = path;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    const QJsonDocument doc(toJson());
    f.write(doc.toJson(QJsonDocument::Indented));
}

QJsonObject AppConfig::toJson() const
{
    QJsonObject j;
    QJsonObject motor;
    motor["defaultSerial"] = data_.defaultSerial;
    motor["positionToUm"] = data_.positionToUm;
    motor["minPositionMm"] = data_.minPositionMm;
    motor["maxPositionMm"] = data_.maxPositionMm;
    motor["defaultMaxVelocity"] = data_.defaultMaxVelocity;
    motor["defaultAcceleration"] = data_.defaultAcceleration;
    j["motor"] = motor;

    QJsonObject log;
    log["dir"] = data_.logDir;
    log["level"] = data_.logLevel;
    log["maxFileSize"] = static_cast<qint64>(data_.logMaxFileSize);
    j["log"] = log;

    QJsonObject camera;
    camera["userId"] = data_.cameraUserId;
    camera["imageSaveDir"] = data_.imageSaveDir;
    j["camera"] = camera;

    j["language"] = data_.language;
    return j;
}

void AppConfig::fromJson(const QJsonObject &j)
{
    // 读取时保留默认值，只覆盖出现的字段
    const QJsonObject motor = j.value("motor").toObject();
    if (motor.contains("defaultSerial"))
        data_.defaultSerial = motor["defaultSerial"].toString();
    if (motor.contains("positionToUm"))
        data_.positionToUm = motor["positionToUm"].toDouble(data_.positionToUm);
    if (motor.contains("minPositionMm"))
        data_.minPositionMm = motor["minPositionMm"].toDouble(data_.minPositionMm);
    if (motor.contains("maxPositionMm"))
        data_.maxPositionMm = motor["maxPositionMm"].toDouble(data_.maxPositionMm);
    if (motor.contains("defaultMaxVelocity"))
        data_.defaultMaxVelocity = static_cast<int>(motor["defaultMaxVelocity"].toInt(data_.defaultMaxVelocity));
    if (motor.contains("defaultAcceleration"))
        data_.defaultAcceleration = static_cast<int>(motor["defaultAcceleration"].toInt(data_.defaultAcceleration));

    const QJsonObject log = j.value("log").toObject();
    if (log.contains("dir"))
        data_.logDir = log["dir"].toString(data_.logDir);
    if (log.contains("level"))
        data_.logLevel = log["level"].toString(data_.logLevel);
    if (log.contains("maxFileSize"))
        data_.logMaxFileSize = log["maxFileSize"].toVariant().toLongLong();

    const QJsonObject camera = j.value("camera").toObject();
    if (camera.contains("userId"))
        data_.cameraUserId = camera["userId"].toString();
    if (camera.contains("imageSaveDir"))
        data_.imageSaveDir = camera["imageSaveDir"].toString(data_.imageSaveDir);

    if (j.contains("language"))
        data_.language = j["language"].toString(data_.language);
}

} // namespace mc
