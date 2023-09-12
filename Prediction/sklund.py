import tensorflow as tf
import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler

# Load the CSV data file into a Pandas dataframe
data = pd.read_csv('stock_pre.csv', encoding='ISO-8859-1')

# Standardize the input features
scaler = StandardScaler()
data.iloc[:, :-1] = scaler.fit_transform(data.iloc[:, :-1])
model = tf.keras.models.load_model('ref_part.h5')
new_data = np.array([[4,0.4,900,720,800,0.8]])
new_data = scaler.transform(new_data)
prediction = model.predict(new_data)

print('Predicted next maintenance date in days:', prediction[0][0])
