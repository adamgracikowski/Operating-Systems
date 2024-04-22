using System;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using System.Collections.ObjectModel;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Threading;

namespace ChatClient;

public partial class MainWindow : Window
{
    public Client? Client { get; set; }
    public string Username { get; set; } = "";
    public bool Connected => Client is not null && Client.Connected;
    public ObservableCollection<object> Messages { get; }

    public MainWindow()
    {
        Messages = new ObservableCollection<object>();
        InitializeComponent();
        DataContext = this;
    }

    private void OnKeyDown(object? sender, KeyEventArgs e)
    {
        if (e is { Key: Key.Enter })
        {
            e.Handled = true;
            if (e is { KeyModifiers: not KeyModifiers.Shift })
            {
                OnSend(sender, e);
            }
            else if(sender is TextBox textbox)
            {
                textbox.Text = textbox.Text?.Insert(textbox.CaretIndex, textbox.NewLine);
                textbox.CaretIndex += textbox.NewLine.Length;
            }
        }
    }

    private async void OnSend(object? sender, RoutedEventArgs e)
    {
        var text = MessageTextBox?.Text;
        if (text is not null)
        {
            MessageTextBox?.Clear();
            var message = new ChatMessage("You", text, DateTime.Now);
            AddMessage(message);
            if (Connected)
            {
                await Task.Run(() => SendMessage(message));
            }
        }
    }

    private void AddMessage(object message)
    {
        Messages.Add(message);
        var container = MessagesControl.ContainerFromItem(message);
        MessagesControl.UpdateLayout();
        container?.BringIntoView();
    }

    private async void OnConnect(object? sender, RoutedEventArgs e)
    {
        var connectWindow = new ConnectWindow();
        await connectWindow.ShowDialog(this);
        if (connectWindow.Client is not null)
        {
            Client = connectWindow.Client;
            Username = connectWindow.Username;
            if (Client.Connected)
            {
                AddMessage(new StatusMessage("Connected"));
                
                ConnectMenuItem.IsEnabled = false;
                await Task.Run(ReadMessages);
                ConnectMenuItem.IsEnabled = true;
                
                AddMessage(new StatusMessage("Disconnected"));
            }
        }
    }

    private async void OnDisconnect(object? sender, RoutedEventArgs e)
    {
        if (Client is null) return;
        await Task.Run(Client.TcpClient.Close);
    }

    private void SendMessage(ChatMessage message)
    {
        var client = Client;
        if (client == null) return;
        
        byte[] buffer = new byte[Client.BuffSize];
        try
        {
            Encoding.ASCII.GetBytes(Username, 0, Math.Min(Client.NameSize, Username.Length), buffer, Client.NameOffset);
            Encoding.ASCII.GetBytes(message.Text, 0, Math.Min(Client.MessageSize, message.Text.Length), buffer, Client.MessageOffset);
            client.NetworkStream.Write(buffer);
            client.NetworkStream.Flush();
        }
        catch (Exception e)
        {
            client.TcpClient.Close();
        }
    }
    
    private void ReadMessages()
    {
        var client = Client;
        if (client == null) return;
        var buffer = new byte[Client.BuffSize];
        try
        {
            while (client.Connected)
            {
                client.NetworkStream.ReadExactly(buffer);
                var name = Encoding.ASCII.GetString(buffer, Client.NameOffset, Client.NameSize).TrimEnd('\0');
                var message = Encoding.ASCII.GetString(buffer, Client.MessageOffset, Client.MessageSize).TrimEnd('\0');
                Dispatcher.UIThread.Invoke(() => AddMessage(new ChatMessage(name, message, DateTime.Now)));
            }
        }
        catch (Exception e)
        {
            // ignored
        }
        finally
        {
            client.TcpClient.Close();
        }
    }

    private void OnExit(object? sender, RoutedEventArgs e)
    {
        Close();
    }

    private void OnClosing(object? sender, WindowClosingEventArgs e)
    {
        if (Connected)
        {
            Client?.TcpClient.Close();
        }
    }
}