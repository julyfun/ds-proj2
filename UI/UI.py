# author: Benjamin2406ly
# datetime: 2024/6/1

import pandas as pd
import plotly.express as px
from dash import Dash, html, dcc

external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']
app = Dash(__name__, external_stylesheets=external_stylesheets)
# app = Dash()

colors = {
    'background': '#111111',
    'text': '#7FDBFF'
}  # 预设样式

df = pd.DataFrame({'x': [1, 2, 3], 'SF': [4, 1, 2], 'Montreal': [2, 4, 5]})  # 原始数据

fig = px.bar(df, x='x', y=['SF', 'Montreal'], barmode='group')  # 柱状图

fig.update_layout(
    plot_bgcolor=colors['background'],
    paper_bgcolor=colors['background'],
    font_color=colors['text']
)  # 更新柱状图样式

app.layout = html.Div(
    style={'backgroundColor': colors['background']},  # 全局样式
    children=[
        html.H1(
            children='Hello Dash',
            style={
                'textAlign': 'center',
                'color': colors['text']
            }
        ),

        html.Div(
            children='Dash: 一款Python web应用框架',
            style={
                'textAlign': 'center',
                'color': colors['text']
            }
        ),

        dcc.Graph(
            id='example-graph-2',
            figure=fig
        )
    ])

if __name__ == '__main__':
    app.run_server(debug=True)
