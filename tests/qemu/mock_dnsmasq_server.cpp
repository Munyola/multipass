/*
 * Copyright (C) 2018-2019 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>

const auto unexpected_error = 5;

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    QCommandLineOption pidOption("pid-file", "Path for the pid file", "path");
    QCommandLineOption listenOption("listen-address", "Address to listen on", "address");

    parser.addOptions({pidOption, listenOption});

    parser.parse(QCoreApplication::arguments());

    if (parser.isSet(listenOption))
    {
        auto address = parser.value(listenOption);
        // For testing, we treat a 0.0.0 subnet as an error
        if (address.contains("0.0.0"))
        {
            return 1;
        }
    }

    int pipefd[2];
    if (pipe(pipefd))
    {
        std::cerr << "Failed to create pipe: " << std::strerror(errno) << std::endl;
        return unexpected_error;
    }

    char exit_code;

    auto pid = fork();
    if (pid == 0)
    {
        close(pipefd[0]);

        if (parser.isSet(pidOption))
        {
            QFile pid_file{parser.value(pidOption)};
            pid_file.open(QIODevice::WriteOnly);
            pid_file.write(QString::number(getpid()).toUtf8());
        }

        if (write(pipefd[1], "0", 1) < 1)
        {
                std::cerr << "Failed to write to pipe: " << std::strerror(errno) << std::endl;
                return unexpected_error;
        }

        close(pipefd[1]);

        return QCoreApplication::exec();
    }
    else if (pid > 0)
    {
        close(pipefd[1]);

        if (read(pipefd[0], &exit_code, 1) < 1)
        {
            std::cerr << "Failed to read from pipe: " << std::strerror(errno) << std::endl;
            return unexpected_error;
        }

        close(pipefd[0]);

        return atoi(&exit_code);
    }
}
