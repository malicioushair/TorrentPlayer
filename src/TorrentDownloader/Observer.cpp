#include "Observer.h"

#include "Notifier.h"

IObserver::IObserver(Notifier & notifier)
	: m_notifier(notifier)
{
	m_notifier.RegisterObserver(this);
}

IObserver::~IObserver()
{
	m_notifier.UnregisterObserver(this);
}