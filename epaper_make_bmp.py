import json
from datetime import datetime, time
from time import strftime

import requests
import os
import locale
import time

from PIL import Image
from PIL import ImageFont
from PIL import ImageDraw

KEY = "enter your ket here"


class temps:
    def __init__(self, morning, day, evening, cmorning, cday, cevening):
        self.morning = morning
        self.day = day
        self.evening = evening
        self.cmorning = cmorning
        self.cday = cday
        self.cevening = cevening


def main():
    locale.setlocale(locale.LC_ALL, 'ru-Ru')

    current_workdir = os.getcwd()
    workdir = "C:/Users/atomrdp/Documents/My Web Sites/LightControl/"
    os.chdir(workdir)
    
    x = open('e_paper_last_data.txt', 'r')
    fnd_string = x.readline()
    x.close()

    #print(fnd_string)

    Vcc = fnd_string[20:28]

    #print(Vcc)

    os.chdir(current_workdir)

    
    # if not os.path.exists('data'):
    lat = "55.75396"
    lon = "37.620393"
    
    data = requests.get(f'https://api.weather.yandex.ru/v2/informers?lat={lat}&lon={lon}&lang=ru_RU',
                            headers={"X-Yandex-API-Key": KEY}).json()

    with open('data', 'w') as tfile:
            tfile.write(json.dumps(data))
    # else:
    #     data = json.loads(open('data', 'r').readlines()[0])

    osadki_ru = {
        'clear': 'ясно',
        'partly-cloudy': 'малооблачно',
        'cloudy': 'облачно с прояснениями',
        'overcast': 'пасмурно',
        'drizzle': 'морось',
        'light-rain': 'небольшой дождь',
        'rain': 'дождь',
        'moderate-rain': 'умеренно сильный дождь',
        'heavy-rain': 'сильный дождь',
        'continuous-heavy-rain': 'длительный сильный дождь',
        'showers': 'ливень',
        'wet-snow': 'дождь со снегом',
        'light-snow': 'небольшой снег',
        'snow': 'снег',
        'snow-showers': 'снегопад',
        'hail': 'град',
        'thunderstorm': 'гроза',
        'thunderstorm-with-rain': 'дождь с грозой',
        'thunderstorm-with-hail': 'гроза с градом'
    }

    day_part_ru = {
        'night': 'ночь',
        'morning': 'утро',
        'day': 'день',
        'evening': 'вечер'
    }

    moon_ru = {
        'moon-code-0': 'полнолуние',
        'moon-code-1': 'убывающая луна',
        'moon-code-2': 'убывающая луна',
        'moon-code-3': 'убывающая луна',
        'moon-code-4': 'последняя четверть',
        'moon-code-5': 'убывающая луна',
        'moon-code-6': 'убывающая луна',
        'moon-code-7': 'убывающая луна',
        'moon-code-8': 'новолуние',
        'moon-code-9': 'растущая луна',
        'moon-code-10': 'растущая луна',
        'moon-code-11': 'растущая луна',
        'moon-code-12': 'первая четверть',
        'moon-code-13': 'растущая луна',
        'moon-code-14': 'растущая луна',
        'moon-code-15': 'растущая луна'
            }

    temp = data['fact']['temp']
    feels_like = data['fact']['feels_like']
    pressure_mm = data['fact']['pressure_mm']
    humidity = data['fact']['humidity']
    wind_speed = data['fact']['wind_speed']
    condition = osadki_ru[data['fact']['condition']].capitalize()
    moon = moon_ru[data['forecast']['moon_text']].capitalize()

    part_name0 = day_part_ru[data["forecast"]["parts"][0]['part_name']].capitalize()
    temp0 = data["forecast"]["parts"][0]['temp_avg']
    temp0_as = data["forecast"]["parts"][0]['feels_like']
    condition0 = osadki_ru[data["forecast"]["parts"][0]['condition']].capitalize()
    veter0 = data["forecast"]["parts"][0]['wind_speed']
    pressure_mm0 = data["forecast"]["parts"][0]['pressure_mm']
    humidity0 = data["forecast"]["parts"][0]['humidity']

    part_name1 = day_part_ru[data["forecast"]["parts"][1]['part_name']].capitalize()
    temp1 = data["forecast"]["parts"][1]['temp_avg']
    condition1 = osadki_ru[data["forecast"]["parts"][1]['condition']].capitalize()
    temp1_as = data["forecast"]["parts"][1]['feels_like']
    veter1 = data["forecast"]["parts"][1]['wind_speed']
    pressure_mm1 = data["forecast"]["parts"][1]['pressure_mm']
    humidity1 = data["forecast"]["parts"][1]['humidity']

    sunrise = data["forecast"]['sunrise']
    sunset = data["forecast"]['sunset']

    img = Image.new('RGB', (300, 400), "white")
    draw = ImageDraw.Draw(img)

    posY = 0
    deltaY = 42

    s = Vcc + " " + time.strftime("%H:%M:%S", time.localtime())
    font = ImageFont.FreeTypeFont("Mozer-SemiBold.otf", 18)
    draw.text((190, 0), s, (0, 0, 0), font=font)

    s = time.strftime("%d %b, %A", time.localtime())
    font = ImageFont.FreeTypeFont("Mozer-SemiBold.otf", 40)
    draw.text((0, posY), s, (255, 0, 0), font=font)
    posY += deltaY
    deltaY = 32
    font = ImageFont.FreeTypeFont("Mozer-SemiBold.otf", 32)

    draw.text((0, posY), f"Там {temp}°C, как {feels_like}°C,", (255, 0, 0), font=font)
    posY += deltaY

    draw.text((0, posY), f"Влажность: {humidity}%", (255, 0, 0), font=font)
    posY += deltaY

    draw.text((0, posY), f"Давл {pressure_mm} мм, Ветер {wind_speed} м\\с", (255, 0, 0), font=font)
    posY += deltaY

    draw.text((0, posY), f"{condition}", (255, 0, 0), font=font)
    posY += deltaY

    deltaY = 23
    font = ImageFont.FreeTypeFont("Mozer-SemiBold.otf", deltaY)
    posY += deltaY/4

    draw.text((0, posY), f"Восход: {sunrise} Закат: {sunset}", (255, 0, 0), font=font)
    posY += deltaY

    draw.text((0, posY), f"{moon}", (255, 0, 0), font=font)
    posY += deltaY
    posY += deltaY/4
    draw.text((0, posY), f"Прогноз:", (0, 0, 0), font=font)
    posY += deltaY
    posY += deltaY / 8

    #font = ImageFont.FreeTypeFont("Mozer-SemiBold.otf", 32)
    font = ImageFont.FreeTypeFont("Mozer-SemiBoldItalic.otf", deltaY)
    #deltaY = 24
    draw.text((0, posY), f"{part_name0}: {temp0}°C как {temp0_as}°C, Влажн: {humidity0}%", (0, 0, 0),
              font=font)
    posY += deltaY
    draw.text((0, posY), f"{condition0},", (0, 0, 0),
              font=font)
    posY += deltaY
    draw.text((0, posY), f"Давл: {pressure_mm0} мм, Ветер: {veter0} м\\с", (0, 0, 0),
              font=font)
    posY += deltaY

    posY += deltaY / 8

    draw.text((0, posY), f"{part_name1}: {temp1}°C как {temp1_as}°C, Влажн: {humidity1}%", (0, 0, 0),
              font=font)
    posY += deltaY
    draw.text((0, posY), f"{condition1},", (0, 0, 0),
              font=font)
    posY += deltaY
    draw.text((0, posY), f"Давл: {pressure_mm1} мм, Ветер: {veter1} м\\с", (0, 0, 0),
              font=font)
    #img.show()
    img.rotate(90, expand=True).save('sample-out.bmp')
 
    img = Image.new('RGB', (300, 400), "white")
    draw = ImageDraw.Draw(img)

    posY = 0
    deltaY = 40

    s = time.strftime("%d %b, %A", time.localtime())
    font = ImageFont.FreeTypeFont("Mozer-SemiBold.otf", deltaY)
    draw.text((0, posY), s, (0, 0, 0), font=font)
    posY += deltaY

    s = time.strftime("%H:%M:%S", time.localtime()) + "  " + Vcc
    font = ImageFont.FreeTypeFont("Mozer-SemiBold.otf", deltaY)
    draw.text((0, posY), s, (0, 0, 0), font=font)
    posY += deltaY

    deltaY = 86
    font = ImageFont.FreeTypeFont("Mozer-SemiBold.otf", deltaY)
    draw.text((0, posY), 'Батарею', (255, 0, 0), font=font)
    posY += deltaY

    draw.text((0, posY), 'надо', (255, 0, 0), font=font)
    posY += deltaY

    draw.text((0, posY), 'зарядить!', (255, 0, 0), font=font)
    posY += deltaY

    img.save('batt1.bmp')
    img.rotate(90, expand=True).save('batt.bmp')

    # img.show()


if __name__ == "__main__":
    main()
