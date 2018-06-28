long getRawAirQuality(int ADCPin)
{
    int16_t sensor_value = analogRead(ADCPin);

  sensor_value =  sensor_value*32;  // /3.2
  return sensor_value;
}


int getAirQuality(int ADCPin)
{


  int16_t sensor_value = analogRead(ADCPin);

  sensor_value = sensor_value*32;  // /3.2


  if (sensor_value > 11200)
  {

    return 4;
  }
  else if (sensor_value > 6400)
  {

    return 3;
  }
  else if (sensor_value > 4800)
  {

    return 2;
  }
  else if (sensor_value > 3200)
  {

    return 1;
  }
  else
  {

    return 0;

  }

  return -1;
}

String AirQualityTextFromRaw(long sensor_value)
{



  if (sensor_value > 11200)
  {
    return "Very High Pollution Detected";
  }
  else if (sensor_value > 6400)
  {
    return "High Pollution";
  }
  else if (sensor_value > 4800)
  {

    return "Medium Pollution";
  }
  else if (sensor_value > 3200)
  {

    return "Low Pollution";
  }
  else
  {

    return "Fresh Air";

  }

  return "UNKNOWN";


}


 
