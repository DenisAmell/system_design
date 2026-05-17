workspace "Taxi Ordering System" "Система заказа такси (вариант 16)" {

    !identifiers hierarchical

    model {

        passenger = person "Пассажир" "Зарегистрированный пользователь, заказывающий поездки через мобильное приложение."
        driver    = person "Водитель" "Зарегистрированный водитель, принимающий заказы и выполняющий поездки."
        admin     = person "Администратор" "Сотрудник, управляющий пользователями, водителями и контентом системы."

        paymentSystem = softwareSystem "Платёжная система" "Внешний платёжный шлюз (например, Stripe / ЮKassa) для списания средств за поездку." {
            tags "ExternalSystem"
        }
        smsGateway = softwareSystem "SMS-шлюз" "Внешний сервис отправки SMS для подтверждения регистрации и уведомлений." {
            tags "ExternalSystem"
        }
        pushService = softwareSystem "Push-сервис" "Apple APNs / Google FCM — доставка push-уведомлений на мобильные устройства." {
            tags "ExternalSystem"
        }
        mapsService = softwareSystem "Сервис карт и геолокации" "Внешний сервис карт (Yandex Maps / Google Maps) для построения маршрутов, расчёта стоимости и ETA." {
            tags "ExternalSystem"
        }

        taxiSystem = softwareSystem "Система заказа такси" "Платформа заказа такси: регистрация пользователей и водителей, создание заказов, матчинг водителей и пассажиров, выполнение поездок и история." {

            passengerApp = container "Мобильное приложение пассажира" "Позволяет пассажиру зарегистрироваться, заказать поездку, посмотреть историю." "iOS / Android (Swift, Kotlin)" {
                tags "MobileApp"
            }
            driverApp = container "Мобильное приложение водителя" "Позволяет водителю регистрироваться, получать активные заказы, принимать их, завершать поездки." "iOS / Android (Swift, Kotlin)" {
                tags "MobileApp"
            }
            adminWeb = container "Веб-панель администратора" "Управление пользователями, водителями, мониторинг поездок." "TypeScript, React" {
                tags "WebApp"
            }

            apiGateway = container "API Gateway" "Единая точка входа для клиентских приложений: маршрутизация запросов, аутентификация, rate limiting." "C++ Userver" {
                tags "Gateway"
            }
            
            userService = container "User Service" "Управление учётными записями пользователей: регистрация, поиск по логину, поиск по маске ФИО, аутентификация." "C++ Userver" {
                tags "Service"
            }
            driverService = container "Driver Service" "Регистрация водителей, хранение профиля, статусов (свободен/занят/офлайн) и текущей геопозиции." "C++ Userver" {
                tags "Service"
            }
            rideService = container "Ride Service" "Создание заказов поездок, изменение статусов (создан/принят/в пути/завершён), история поездок пользователя." "C++ Userver" {
                tags "Service"
            }
            matchingService = container "Matching Service" "Подбор ближайшего свободного водителя для нового заказа, публикация активных заказов." "C++ Userver" {
                tags "Service"
            }
            notificationService = container "Notification Service" "Доставка push/SMS-уведомлений пользователям и водителям (новый заказ, принят, завершён)." "C++ Userver" {
                tags "Service"
            }

            userDb = container "User Database" "Хранение учётных данных пользователей и водителей." "PostgreSQL" {
                tags "Database"
            }
            rideDb = container "Ride Database" "Хранение заказов поездок и их истории." "PostgreSQL" {
                tags "Database"
            }
            cache = container "Cache / Geo Store" "Кэш активных заказов, текущих геопозиций водителей (геоиндекс)." "Redis" {
                tags "Cache"
            }
            broker = container "Message Broker" "Шина асинхронных событий между сервисами: новый заказ, принятие заказа, завершение." "Apache Kafka" {
                tags "Broker"
            }

            passengerApp -> apiGateway "Использует REST API" "HTTPS/JSON"
            driverApp    -> apiGateway "Использует REST API" "HTTPS/JSON"
            adminWeb     -> apiGateway "Использует REST API" "HTTPS/JSON"

            apiGateway -> userService     "Маршрутизирует запросы по пользователям" "HTTP/JSON"
            apiGateway -> driverService   "Маршрутизирует запросы по водителям"     "HTTP/JSON"
            apiGateway -> rideService     "Маршрутизирует запросы по поездкам"      "HTTP/JSON"

            userService     -> userDb  "Читает/пишет учётные данные" "TCP, libpqxx"
            driverService   -> userDb  "Читает/пишет данные водителей" "TCP, libpqxx"
            driverService   -> cache   "Сохраняет/обновляет геопозиции водителей" "RESP, redis-plus-plus"
            rideService     -> rideDb  "Читает/пишет данные о поездках" "TCP, libpqxx"
            rideService     -> cache   "Хранит активные заказы" "RESP, redis-plus-plus"
            matchingService -> cache   "Читает геопозиции и активные заказы" "RESP, redis-plus-plus"

            rideService         -> broker        "Публикует события (RideCreated, RideCompleted)" "Kafka protocol"
            matchingService     -> broker        "Подписан на RideCreated, публикует DriverMatched" "Kafka protocol"
            notificationService -> broker        "Подписан на события поездок" "Kafka protocol"
            rideService         -> driverService "Проверяет/резервирует водителя" "HTTP/JSON"

            notificationService -> pushService   "Отправляет push-уведомления" "HTTPS"
            notificationService -> smsGateway    "Отправляет SMS" "HTTPS"
            rideService         -> paymentSystem "Списывает оплату по завершении поездки" "HTTPS/REST"
            rideService         -> mapsService   "Строит маршрут, считает стоимость и ETA" "HTTPS/REST"
            matchingService     -> mapsService   "Считает расстояние до пассажира" "HTTPS/REST"
        }

        passenger -> taxiSystem.passengerApp "Заказывает поездки"
        driver    -> taxiSystem.driverApp    "Принимает и выполняет заказы"
        admin     -> taxiSystem.adminWeb     "Администрирует систему"

        pushService -> passenger "Доставляет push-уведомления"
        pushService -> driver    "Доставляет push-уведомления"
        smsGateway  -> passenger "Доставляет SMS"
        smsGateway  -> driver    "Доставляет SMS"
    }

    views {
        systemContext taxiSystem "SystemContext" "Контекстная диаграмма системы заказа такси" {
            include *
            autoLayout lr
        }

        container taxiSystem "Containers" "Контейнерная диаграмма системы заказа такси" {
            include *
            autoLayout lr
        }

        dynamic taxiSystem "CreateAndAcceptRide" "Сценарий: пассажир создаёт заказ, система подбирает водителя, водитель принимает заказ" {
            passenger -> taxiSystem.passengerApp "Нажимает «Заказать такси»"
            taxiSystem.passengerApp -> taxiSystem.apiGateway "POST /rides — создать заказ"
            taxiSystem.apiGateway -> taxiSystem.rideService "Маршрутизирует запрос создания заказа"
            taxiSystem.rideService -> mapsService "Считает маршрут, стоимость, ETA"
            taxiSystem.rideService -> taxiSystem.rideDb "Сохраняет заказ в статусе CREATED"
            taxiSystem.rideService -> taxiSystem.cache "Кладёт заказ в список активных"
            taxiSystem.rideService -> taxiSystem.broker "Публикует событие RideCreated"
            taxiSystem.broker -> taxiSystem.matchingService "Доставляет RideCreated"
            taxiSystem.matchingService -> taxiSystem.cache "Ищет ближайшего свободного водителя по геоиндексу"
            taxiSystem.matchingService -> taxiSystem.broker "Публикует DriverCandidateSelected"
            taxiSystem.broker -> taxiSystem.notificationService "Доставляет DriverCandidateSelected"
            taxiSystem.notificationService -> pushService "Шлёт push водителю о новом заказе"
            pushService -> driver "Push: «Новый заказ»"
            driver -> taxiSystem.driverApp "Нажимает «Принять»"
            taxiSystem.driverApp -> taxiSystem.apiGateway "POST /rides/{id}/accept"
            taxiSystem.apiGateway -> taxiSystem.rideService "Маршрутизирует принятие заказа"
            taxiSystem.rideService -> taxiSystem.driverService "Резервирует водителя (статус BUSY)"
            taxiSystem.rideService -> taxiSystem.rideDb "Меняет статус заказа на ACCEPTED"
            taxiSystem.rideService -> taxiSystem.broker "Публикует RideAccepted"
            taxiSystem.broker -> taxiSystem.notificationService "Доставляет RideAccepted"
            taxiSystem.notificationService -> pushService "Шлёт push пассажиру"
            pushService -> passenger "Push: «Водитель найден»"
            autoLayout lr
        }

        styles {
            element "Person" {
                shape Person
                background #08427b
                color #ffffff
            }
            element "Software System" {
                background #1168bd
                color #ffffff
            }
            element "ExternalSystem" {
                background #999999
                color #ffffff
            }
            element "Container" {
                background #438dd5
                color #ffffff
            }
            element "MobileApp" {
                shape MobileDevicePortrait
            }
            element "WebApp" {
                shape WebBrowser
            }
            element "Gateway" {
                shape Hexagon
                background #2e7d32
            }
            element "Service" {
                shape RoundedBox
                background #438dd5
            }
            element "Database" {
                shape Cylinder
                background #2d6a4f
            }
            element "Cache" {
                shape Cylinder
                background #b08968
            }
            element "Broker" {
                shape Pipe
                background #6a4c93
            }
        }

        theme default
    }
}
