using System;

namespace ChatClient;

public class ChatMessage
{
    public string Sender { get; set; }
    public string Text { get; set; }
    public DateTime Time { get; set; }

    public ChatMessage(string sender, string text, DateTime time)
    {
        Sender = sender;
        Text = text;
        Time = time;
    }
}