# injector

injector - это консольная программа, для инжектирования DLL библиотек. Инжектирование может проводиться по PID, или по названию процесса.
Вместе с инжектором также есть тестовая программа и тестовая DLL библиотека.
Проект разработан в среде Visual Studio 2019.

    Usage:
        injector[64].exe"
            --dll [path to dll file]"

            --pid [process id]"
                or"
            --process-name [process name]"
