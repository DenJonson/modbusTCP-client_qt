<h1>
    Клиент, использующий протокл связи MODBUS-TCP
  </h1>
  <p>
    Данный проект реализует MODBUS-TCP клиент, позволяющий установить соединение с MODBUS-TCP сервером и отправить ему запрос, получив ответ.
  </p>
  <p>Данный клиент рекомендуется использовать в связке с сервером: https://github.com/DenJonson/modbusTCP-server_qt (на других не проверялся) </p>
 <p>Клиент разработан на основе информации с сайта: https://ru.wikipedia.org/wiki/Modbus </p>

 <h2>
  Инструкция эксплуатации
 </h2>
 <ol>
  <li>
    <h3>Запуск</h3>
    <p>При запуске клиента откроектся окно, из которого можно подключиться к серверу и отправить запрос.</p>
  </li>
  <li>
    <h3>Подключение</h3>
    <p>В панели "Подключение к серверу находятся два поля: с адресом сервера для подключения и его портом.</p>
    <p>В выпадающем списке адресов будут предложены варианты подключения, однако также можно ввести произвольный адрес. При зупуске рекомендованного сервера и данного клиента стоит подключиться к адресу выбранному по умолчанию</p>
    <p>
      При нажатии кнопки "Подключиться" будет произведена попытка подключения по указанным данным. Если попытка будет успешна, то активируется кнопка "Отправить запрос" и кнопка "Подключиться" будте окрашщена в зеленый цвет. Иначе кнопка "Подключиться" будет окрашена в красный
    </p>
  </li>
  <li>
    <h3>Отправка запроса</h3>
    <p>В поле "Произвольный запрос" находятся поля, в которые можно вписать modbusTCP команду в шестнадцатиричном формате. Есть два фиксированных поля "Адрес устройства" и "Код функции", а также поле "Команда" с варьирующимся количестовм полей, что, в теории, делает клиент более универсальным</p>
    <p>
      При отправке запрос формируется следующим образом:
    </p>
    <p>
      номер транзакции - номер протокола - размер команды - (адрес устройства - код функции - команда) - CRC16
    </p>
  </li>
  <li>
    <h3>Примеры запросов к рекомендованному серверу:</h3>
    <ol>
      <li><h4>Чтение нескольких дискретных выходов - 1</h4>
        <p>
          00 01 4E 9A 00 02
        </p>
        <p>
          Позволяет прочитать 2 поля дисквретных выходов с адреса 0x4E9A
        </p>
      </li>
      <li><h4>Чтение нескольких дискретных входов - 2</h4>
        <p>
          00 02 27 2F 00 03
        </p>
        <p>
          Позволяет прочитать 3 поля дискретных входов с адреса 0x272F
        </p>
      </li>
      <li><h4>Чтение 32-битных выходов - 3</h4>
        <p>
          00 03 9C 7A 00 02
        </p>
      <p>
        Позволяет прочитать два поля 32-б выходов
      </p>
    </li>
      <li><h4>Чтение 16-битных входов - 4</h4>
        <p>
          00 04 77 73 00 02
        </p>
        <p>
          Позволяет прочитать два поля 16-битных выходов начиная с адреса 0x7773
        </p>
      </li>
      <li><h4>Запись одного дискретного выхода - 5</h4><p>
        00 05 4E 9B FF 00 
      </p>
      <p>
        Выставляет на втором дискретном выходе единицу
      </p></li>
      <li><h4>Запись одного 32-битного выхода - 6</h4>
        <p>
          00 06 9C 7A 00 02 08 00
        </p>
        <p>
          Записывает в первый 32-б выход значение: 0x00020800
        </p>
      </li>
      <li><h4>Запись нескольких дискретных выходов - 0xF</h4>
        <p>
          00 0F 4E 9A 00 04 01 05
        </p>
      <p>
        Записывает в поля дискретных выводов данные: 1 0 1 0
      </p></li>
      <li><h4>Запись нескольких 32-битных выходов - 0x10</h4>
        <p>
          00 10 9C 7B 00 02 08 00 00 02 40 00 00 00 01
      </p>
    <p>
      Записывает в поля 32-б выходов значения: 0x1, 0x240
    </p>
    </li>
    </ol>
  </li>
 </ol>
