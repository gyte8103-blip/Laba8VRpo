#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include<locale.h>
#include <optional>
#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <windows.h>
struct Customer {
	int id;
	std::string name;
	std::string contacts;
};
struct Manager {
	int id;
	std::string fullName;
};
struct Developer {
	int id;
	std::string fullName;
	std::string qualification;
	double rate = 1000.0;
	bool busy = false;
	bool isFree() const { return !busy; }
	void take() { busy = true; }
	void release() { busy = false; }
};
struct Work {
	int id;
	std::string title;
	std::string qualification;
	int count = 0;
	double plannedHours = 0.0;
};
struct TechTask
{
	int id;
	int customerId;
	std::vector<Work> works;
	std::string status = "новое";
};
struct Assignment
{
	int id;
	int projectId;
	int developerId;
	double actualHours = 0.0;
	void addHours(double h)
	{
		if (h <= 0) throw std::invalid_argument("Часы должны быть положительными");
		actualHours += h;
	}
};
struct Project
{
	int id;
	std::string title;
	int techTaskId;
	int managerId;
	std::vector<Assignment> assignments;
	double cost = 0.0;
	std::string status = "оформлен";
};
struct Invoice
{
	int id;
	int projectId;
	double amount;
	std::string status = "выставлен";
};
struct Storage
{
	std::vector<Customer>    customers;
	std::vector<Manager>     managers;
	std::vector<Developer>   developers;
	std::vector<TechTask>    techTasks;
	std::vector<Project>     projects;
	std::vector<Assignment>  assignments;
	std::vector<Invoice>     invoices;
	int workSeq = 1;
	int ttSeq = 1;
	int projSeq = 1;
	int assSeq = 1;
	int invSeq = 1;
	Developer* findDeveloper(int id) {
		for (auto& d : developers) if (d.id == id) return &d;
		return nullptr;
	}
	TechTask* findTechTask(int id) {
		for (auto& t : techTasks) if (t.id == id) return &t;
		return nullptr;
	}
	Project* findProject(int id) {
		for (auto& p : projects) if (p.id == id) return &p;
		return nullptr;
	}
};
class ProjectService {
public:
	explicit ProjectService(Storage& s) : st(s) {}
	Project createProject(const std::string& title, TechTask& tt, int managerId) {
		if (tt.works.empty())
			throw std::runtime_error("ТЗ не содержит работ");
		Project p;
		p.id = st.projSeq++;
		p.title = title;
		p.techTaskId = tt.id;
		p.managerId = managerId;
		st.projects.push_back(p);
		tt.status = "в работе";
		return p;
	}
	void assignDevelopers(Project& project, TechTask& tt) {
		std::vector<std::string> errors;
		for (const Work& w : tt.works) {
			std::vector<Developer*> candidates;
			for (auto& d : st.developers) {
				if (d.isFree() && d.qualification == w.qualification)
					candidates.push_back(&d);
			}
			if (static_cast<int>(candidates.size()) < w.count) {
				errors.push_back("«" + w.title + "»: нужно " +
					std::to_string(w.count) + " (" + w.qualification +
					"), свободно " + std::to_string(candidates.size()));
				continue;
			}
			for (int i = 0; i < w.count; ++i) {
				Developer* dev = candidates[i];
				dev->take();
				Assignment a;
				a.id = st.assSeq++;
				a.projectId = project.id;
				a.developerId = dev->id;
				st.assignments.push_back(a);
				project.assignments.push_back(a);
			}
		}
		if (!errors.empty()) {
			std::string msg = "Не хватает специалистов:\n";
			for (auto& e : errors) msg += "  - " + e + "\n";
			throw std::runtime_error(msg);
		}
	}

private:
	Storage& st;
};
class BillingService {
public:
	explicit BillingService(Storage& s) : st(s) {}
	double calculateCost(Project& project) {
		double total = 0.0;
		for (const auto& a : project.assignments) {
			Developer* dev = st.findDeveloper(a.developerId);
			if (!dev) continue;
			total += a.actualHours * dev->rate;
		}
		project.cost = total;
		return total;
	}
	Invoice issueInvoice(Project& project) {
		double amount = calculateCost(project);
		if (amount <= 0.0)
			throw std::runtime_error("Стоимость равна нулю — нет отработанных часов");
		Invoice inv;
		inv.id = st.invSeq++;
		inv.projectId = project.id;
		inv.amount = amount;
		st.invoices.push_back(inv);
		return inv;
	}
private:
	Storage& st;
};
class TimeService {
public:
	explicit TimeService(Storage& s) : st(s) {}
	Assignment& logHours(int developerId, int projectId, double hours) {
		for (auto& a : st.assignments) {
			if (a.developerId == developerId && a.projectId == projectId) {
				a.addHours(hours);
				return a;
			}
		}
		throw std::runtime_error("Разработчик не назначен на этот проект");
	}
private:
	Storage& st;
};
static void printDeveloper(const Developer& d) {
	std::cout << "[" << d.id << "] " << d.fullName
		<< " | " << d.qualification
		<< " | " << d.rate << " руб/ч"
		<< " | " << (d.busy ? "занят" : "свободен") << "\n";
}
static void printTechTask(const TechTask& t) {
	std::cout << "ТЗ #" << t.id << " (заказчик #" << t.customerId << ") — "
		<< t.status << "\n";
	for (const auto& w : t.works) {
		std::cout << "   [" << w.id << "] " << w.title
			<< " | квалификация: " << w.qualification
			<< " | требуется: " << w.count << " чел."
			<< " | план. часы: " << w.plannedHours << "\n";
	}
}
static void printProject(const Project& p) {
	std::cout << std::fixed << std::setprecision(2);
	std::cout << "Проект #" << p.id << " «" << p.title << "»"
		<< " | ТЗ #" << p.techTaskId
		<< " | менеджер #" << p.managerId
		<< " | стоимость: " << p.cost << " руб"
		<< " | " << p.status << "\n";
}
static void printInvoice(const Invoice& inv) {
	std::cout << std::fixed << std::setprecision(2);
	std::cout << "Счёт #" << inv.id << " по проекту #" << inv.projectId
		<< " на " << inv.amount << " руб — " << inv.status << "\n";
}
static void seedDemoData(Storage& st) {
	st.customers.push_back({ 1, "ООО «Ромашка»", "romashka@example.ru" });
	st.managers.push_back({ 1, "Иванов И.И." });
	st.managers.push_back({ 2, "Сидорова А.П." });
	st.developers.push_back({ 1, "Петров П.П.", "Backend-разработчик", 1200.0, false });
	st.developers.push_back({ 2, "Смирнов С.С.", "Backend-разработчик", 1100.0, false });
	st.developers.push_back({ 3, "Кузнецов К.К.", "Frontend-разработчик", 1000.0, false });
	st.developers.push_back({ 4, "Орлов О.О.",   "Frontend-разработчик", 1000.0, false });
	st.developers.push_back({ 5, "Белова Б.Б.",  "Тестировщик",          800.0,  false });
}
static int readInt(const std::string& prompt) {
	while (true) {
		std::cout << prompt;
		std::string line;
		if (!std::getline(std::cin, line)) throw std::runtime_error("EOF");
		try { return std::stoi(line); }
		catch (...) { std::cout << "Ожидалось целое число.\n"; }
	}
}
static double readDouble(const std::string& prompt) {
	while (true) {
		std::cout << prompt;
		std::string line;
		if (!std::getline(std::cin, line)) throw std::runtime_error("EOF");
		try { return std::stod(line); }
		catch (...) { std::cout << "Ожидалось число.\n"; }
	}
}
static std::string readLine(const std::string& prompt) {
	std::cout << prompt;
	std::string line;
	if (!std::getline(std::cin, line)) throw std::runtime_error("EOF");
	return line;
}
static void menuCreateTechTask(Storage& st) {
	int custId = readInt("ID заказчика: ");
	auto it = std::find_if(st.customers.begin(), st.customers.end(),
		[&](const Customer& c) { return c.id == custId; });
	if (it == st.customers.end()) {
		std::cout << "Заказчик не найден.\n";
		return;
	}
	TechTask tt;
	tt.id = st.ttSeq++;
	tt.customerId = custId;
	while (true) {
		std::string title = readLine("Название работы (пусто — закончить): ");
		if (title.empty()) break;
		std::string qual = readLine("Квалификация: ");
		int cnt = readInt("Количество специалистов: ");
		double hours = readDouble("Плановые часы: ");
		Work w;
		w.id = st.workSeq++;
		w.title = title;
		w.qualification = qual;
		w.count = cnt;
		w.plannedHours = hours;
		tt.works.push_back(w);
	}
	if (tt.works.empty()) {
		std::cout << "ТЗ без работ не создано.\n";
		return;
	}
	st.techTasks.push_back(tt);
	std::cout << "ТЗ #" << tt.id << " создано.\n";
	printTechTask(tt);
}
static void menuShowTechTask(Storage& st) {
	int id = readInt("ID ТЗ: ");
	TechTask* tt = st.findTechTask(id);
	if (!tt) { std::cout << "ТЗ не найдено.\n"; return; }
	printTechTask(*tt);
}
static void menuCreateProject(Storage& st, ProjectService& ps) {
	int ttId = readInt("ID ТЗ: ");
	TechTask* tt = st.findTechTask(ttId);
	if (!tt) { std::cout << "ТЗ не найдено.\n"; return; }
	std::string title = readLine("Название проекта: ");
	int mgrId = readInt("ID менеджера: ");
	bool mgrOk = std::any_of(st.managers.begin(), st.managers.end(),
		[&](const Manager& m) { return m.id == mgrId; });
	if (!mgrOk) { std::cout << "Менеджер не найден.\n"; return; }
	try {
		Project p = ps.createProject(title, *tt, mgrId);
		ps.assignDevelopers(p, *tt);
		Project* stored = st.findProject(p.id);
		if (stored) *stored = p;
		std::cout << "Проект #" << p.id
			<< " оформлен, разработчики назначены.\n";
		printProject(*stored);
	}
	catch (const std::exception& e) {
		std::cout << "Ошибка: " << e.what() << "\n";
	}
}
static void menuBill(Storage& st, BillingService& bs) {
	int id = readInt("ID проекта: ");
	Project* p = st.findProject(id);
	if (!p) { std::cout << "Проект не найден.\n"; return; }
	try {
		Invoice inv = bs.issueInvoice(*p);
		std::cout << std::fixed << std::setprecision(2);
		std::cout << "Стоимость проекта: " << p->cost << " руб\n";
		printInvoice(inv);
	}
	catch (const std::exception& e) {
		std::cout << "Ошибка: " << e.what() << "\n";
	}
}
static void menuLogHours(Storage& st, TimeService& ts) {
	int devId = readInt("ID разработчика: ");
	int projId = readInt("ID проекта: ");
	double h = readDouble("Затрачено часов: ");
	try {
		Assignment& a = ts.logHours(devId, projId, h);
		std::cout << "Учтено. Итого по назначению: "
			<< a.actualHours << " ч\n";
	}
	catch (const std::exception& e) {
		std::cout << "Ошибка: " << e.what() << "\n";
	}
}
static void menuShowProjects(const Storage& st) {
	if (st.projects.empty()) { std::cout << "Проектов нет.\n"; return; }
	for (const auto& p : st.projects) printProject(p);
}
static void menuShowInvoices(const Storage& st) {
	if (st.invoices.empty()) { std::cout << "Счетов нет.\n"; return; }
	for (const auto& inv : st.invoices) printInvoice(inv);
}
static void menuShowDevelopers(const Storage& st) {
	if (st.developers.empty()) { std::cout << "Разработчиков нет.\n"; return; }
	for (const auto& d : st.developers) printDeveloper(d);
}
static void printMainMenu() {
	std::cout <<
		"\n=== Система «Команда разработчиков» ===\n"
		"1. Заказчик: представить ТЗ\n"
		"2. Менеджер: просмотреть ТЗ\n"
		"3. Менеджер: оформить проект и назначить разработчиков\n"
		"4. Менеджер: рассчитать стоимость и выставить счёт\n"
		"5. Разработчик: отметить затраченные часы\n"
		"6. Показать все проекты\n"
		"7. Показать все счета\n"
		"8. Показать всех разработчиков\n"
		"0. Выход\n";
}


#include <clocale>

int main()
{
	setlocale(LC_ALL, ".UTF-8");

	Storage st;
	seedDemoData(st);
	ProjectService ps(st);
	BillingService bs(st);
	TimeService    ts(st);
	std::cout << "Демонстрационные данные загружены.\n";
	try
	{
		while (true)
		{
			printMainMenu();
			std::string line;
			std::cout << "Выбор: ";
			if (!std::getline(std::cin, line)) break;
			if (line.empty()) continue;
			int choice = 0;
			try { choice = std::stoi(line); }
			catch (...) { std::cout << "Неверный ввод.\n"; continue; }
			try
			{
				switch (choice)
				{
				case 1: menuCreateTechTask(st);          break;
				case 2: menuShowTechTask(st);            break;
				case 3: menuCreateProject(st, ps);       break;
				case 4: menuBill(st, bs);                break;
				case 5: menuLogHours(st, ts);            break;
				case 6: menuShowProjects(st);            break;
				case 7: menuShowInvoices(st);            break;
				case 8: menuShowDevelopers(st);          break;
				case 0:
					std::cout << "Завершение работы.\n";
					return 0;
				default:
					std::cout << "Неизвестный пункт меню.\n";
				}
			}

			catch (const std::exception& e)

			{
				std::cout << "Ошибка выполнения: " << e.what() << "\n";
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "\nЗавершение: " << e.what() << "\n";
	}
	return 0;
}
